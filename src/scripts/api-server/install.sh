#!/usr/bin/env bash
#
# Deploy the RE7 API server behind nginx (HTTPS only, :8443) + php-fpm.
# Fedora-oriented defaults; override any path with a flag. Run as root.
#
# Usage:
#   sudo ./install.sh [--path DIR] [--server-name NAME] [--port N]
#                     [--nginx-conf DIR] [--fpm-pool DIR] [--user USER]
#                     [--cert FILE --key FILE]      # provide certs (else self-signed)
#   sudo ./install.sh --uninstall
#
set -euo pipefail

APP_NAME="re7-api"
SRC_DIR="$(cd "$(dirname "$0")" && pwd)"

# ---- defaults (override with flags) ----
PORT=8443
SERVER_NAME="$(hostname -f 2>/dev/null || hostname)"
APP_ROOT="/usr/share/nginx/${APP_NAME}"       # nginx-served path; --path to change
NGINX_CONF_DIR="/etc/nginx/conf.d"            # --nginx-conf
FPM_POOL_DIR="/etc/php-fpm.d"                 # --fpm-pool
FPM_SOCK="/run/php-fpm/${APP_NAME}.sock"
LOG_DIR="/var/log/${APP_NAME}"
CERT_DIR="/etc/${APP_NAME}/certs"
NGINX_USER="nginx"; NGINX_GROUP="nginx"
FPM_USER="nginx";   FPM_GROUP="nginx"
SELF_SIGNED=1; CERT=""; KEY=""
UNINSTALL=0

log()  { printf '\033[1;32m>>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m!!\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31mxx\033[0m %s\n' "$*" >&2; exit 1; }

usage() { sed -n '3,14p' "$0"; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --path)         APP_ROOT="$2"; shift 2;;
        --server-name)  SERVER_NAME="$2"; shift 2;;
        --port)         PORT="$2"; shift 2;;
        --nginx-conf)   NGINX_CONF_DIR="$2"; shift 2;;
        --fpm-pool)     FPM_POOL_DIR="$2"; shift 2;;
        --user)         FPM_USER="$2"; FPM_GROUP="$2"; shift 2;;
        --cert)         CERT="$2"; SELF_SIGNED=0; shift 2;;
        --key)          KEY="$2";  SELF_SIGNED=0; shift 2;;
        --uninstall)    UNINSTALL=1; shift;;
        -h|--help)      usage; exit 0;;
        *) die "unknown argument: $1 (see --help)";;
    esac
done

[[ $EUID -eq 0 ]] || die "run as root (sudo)."

render() { # <template> <dest>
    sed \
        -e "s|@@PORT@@|${PORT}|g" \
        -e "s|@@SERVER_NAME@@|${SERVER_NAME}|g" \
        -e "s|@@APP_ROOT@@|${APP_ROOT}|g" \
        -e "s|@@CERT@@|${CERT}|g" \
        -e "s|@@KEY@@|${KEY}|g" \
        -e "s|@@FPM_SOCK@@|${FPM_SOCK}|g" \
        -e "s|@@FPM_USER@@|${FPM_USER}|g" \
        -e "s|@@FPM_GROUP@@|${FPM_GROUP}|g" \
        -e "s|@@NGINX_USER@@|${NGINX_USER}|g" \
        -e "s|@@NGINX_GROUP@@|${NGINX_GROUP}|g" \
        -e "s|@@LOG_DIR@@|${LOG_DIR}|g" \
        "$1" > "$2"
}

reload_services() {
    log "nginx -t"; nginx -t
    log "reloading nginx + php-fpm"
    systemctl reload nginx
    systemctl restart php-fpm
}

do_uninstall() {
    log "removing nginx conf + php-fpm pool (app dir & data left in place)"
    rm -fv "${NGINX_CONF_DIR}/${APP_NAME}.conf" "${FPM_POOL_DIR}/${APP_NAME}.conf"
    reload_services
    log "done. App still at ${APP_ROOT} (remove manually if desired)."
}

do_install() {
    command -v nginx    >/dev/null || die "nginx not found"
    command -v php-fpm  >/dev/null || die "php-fpm not found"
    command -v composer >/dev/null || die "composer not found"
    [[ -d "$NGINX_CONF_DIR" ]] || die "nginx conf dir not found: $NGINX_CONF_DIR (use --nginx-conf)"
    [[ -d "$FPM_POOL_DIR"  ]] || die "php-fpm pool dir not found: $FPM_POOL_DIR (use --fpm-pool)"

    mkdir -p "$APP_ROOT" "$LOG_DIR" "$CERT_DIR" "$(dirname "$FPM_SOCK")"

    # 1) dependencies (dev vendor keeps its symlink — the deploy copy is made below).
    #    Run composer as the invoking user so the source tree isn't left root-owned.
    log "composer install"
    if [[ -n "${SUDO_USER:-}" ]] && [[ "$SUDO_USER" != "root" ]]; then
        # heal ownership of any vendor left root-owned by an earlier run
        chown -R "$SUDO_USER" "$SRC_DIR/vendor" "$SRC_DIR/composer.lock" 2>/dev/null || true
        sudo -u "$SUDO_USER" bash -c "cd '$SRC_DIR' && composer install --no-dev --no-interaction --quiet"
    else
        ( cd "$SRC_DIR" && composer install --no-dev --no-interaction --quiet )
    fi

    # 2) copy app (exclude dev/test/secret)
    log "copying app -> ${APP_ROOT}"
    if command -v rsync >/dev/null; then
        rsync -a --delete \
            --exclude='.git' --exclude='.env' --exclude='tests' \
            --exclude='data' --exclude='install.sh' \
            "$SRC_DIR"/ "$APP_ROOT"/
    else
        cp -a "$SRC_DIR"/. "$APP_ROOT"/
        rm -rf "$APP_ROOT/.git" "$APP_ROOT/tests" "$APP_ROOT/.env"
    fi

    # 2b) re7-lib is a path dependency (symlinked in dev). Replace the deployed
    #     symlink with a REAL copy so the app is self-contained on this host.
    log "vendoring re7-lib as a real copy at ${APP_ROOT}"
    rm -rf "$APP_ROOT/vendor/rateengine/re7-lib"
    cp -aL "$SRC_DIR/../lib/php" "$APP_ROOT/vendor/rateengine/re7-lib"
    rm -rf "$APP_ROOT/vendor/rateengine/re7-lib/tests" "$APP_ROOT/vendor/rateengine/re7-lib/.env"
    [[ -f "$APP_ROOT/vendor/rateengine/re7-lib/src/Db.php" ]] || die "re7-lib copy failed (check ../lib/php)"

    # 3) .env — generate a JWT secret; RE7 DB creds must be filled in by the operator
    if [[ ! -f "$APP_ROOT/.env" ]]; then
        log "writing ${APP_ROOT}/.env (JWT secret generated; EDIT RE7_DB_*)"
        sed -e "s|^JWT_SECRET=.*|JWT_SECRET=$(openssl rand -base64 32)|" \
            -e "s|^API_AUTH_DB=.*|API_AUTH_DB=${APP_ROOT}/data/auth.sqlite|" \
            "$SRC_DIR/.env.example" > "$APP_ROOT/.env"
    else
        warn "${APP_ROOT}/.env exists — left unchanged"
    fi
    chmod 600 "$APP_ROOT/.env"
    mkdir -p "$APP_ROOT/data"

    # 4) TLS — self-signed via the repo's gen_tls_cert.sh, or operator-provided
    if [[ $SELF_SIGNED -eq 1 ]]; then
        log "generating self-signed cert (CN=${SERVER_NAME}) in ${CERT_DIR}"
        "$SRC_DIR/../gen_tls_cert.sh" "$CERT_DIR" "$SERVER_NAME" >/dev/null
        CERT="${CERT_DIR}/server.crt"; KEY="${CERT_DIR}/server.key"
    fi
    [[ -f "$CERT" && -f "$KEY" ]] || die "cert/key missing (CERT=$CERT KEY=$KEY)"
    chmod 600 "$KEY" || true

    # 5) nginx + php-fpm config
    log "writing ${NGINX_CONF_DIR}/${APP_NAME}.conf and ${FPM_POOL_DIR}/${APP_NAME}.conf"
    render "$SRC_DIR/deploy/nginx/re7-api.conf.tmpl"        "${NGINX_CONF_DIR}/${APP_NAME}.conf"
    render "$SRC_DIR/deploy/php-fpm/re7-api.pool.conf.tmpl" "${FPM_POOL_DIR}/${APP_NAME}.conf"

    # 6) auth store
    log "initialising SQLite auth store"
    ( cd "$APP_ROOT" && php bin/re7-api-user.php init )

    # 7) ownership / permissions
    chown -R "${FPM_USER}:${NGINX_GROUP}" "$APP_ROOT" "$LOG_DIR"
    chmod 750 "$APP_ROOT/data"

    # 8) SELinux (Fedora/RHEL): let php-fpm read the app, read/write the SQLite
    #    data dir, and open a network connection to the remote RE7 Postgres.
    if command -v getenforce >/dev/null && [[ "$(getenforce)" != "Disabled" ]]; then
        log "applying SELinux contexts + httpd db-connect boolean"
        if command -v semanage >/dev/null && command -v restorecon >/dev/null; then
            semanage fcontext -a -t httpd_sys_content_t    "${APP_ROOT}(/.*)?"      2>/dev/null || true
            semanage fcontext -a -t httpd_sys_rw_content_t "${APP_ROOT}/data(/.*)?" 2>/dev/null || true
            restorecon -RF "$APP_ROOT"
        else
            chcon -R -t httpd_sys_content_t    "$APP_ROOT"       2>/dev/null || true
            chcon -R -t httpd_sys_rw_content_t "$APP_ROOT/data"  2>/dev/null || true
        fi
        setsebool -P httpd_can_network_connect_db on 2>/dev/null || true
    fi

    reload_services

    cat <<EOF

$(log "installed.")
  app        : ${APP_ROOT}
  url        : https://${SERVER_NAME}:${PORT}/health
  nginx conf : ${NGINX_CONF_DIR}/${APP_NAME}.conf
  fpm pool   : ${FPM_POOL_DIR}/${APP_NAME}.conf
  tls        : ${CERT}

Next:
  1) edit ${APP_ROOT}/.env  -> set RE7_DB_* to your RE7 database
  2) create a user:  cd ${APP_ROOT} && sudo -u ${FPM_USER} php bin/re7-api-user.php add <user> --role admin
  3) test:           curl -k https://${SERVER_NAME}:${PORT}/health
EOF
}

if [[ $UNINSTALL -eq 1 ]]; then do_uninstall; else do_install; fi
