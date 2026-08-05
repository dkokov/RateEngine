#!/bin/sh
#
# Entrypoint for the re7-api-server container: mint a self-signed cert, write
# .env from environment (never bake secrets into the image), initialise the
# SQLite auth store + seed an admin, then run php-fpm + nginx.
#
set -e
APP=/app
CN="${SERVER_NAME:-re7-api-server}"

# 1) self-signed TLS cert (once; persisted only if /app/certs is a volume)
if [ ! -f "$APP/certs/server.crt" ]; then
    echo "[re7-api] generating self-signed cert (CN=$CN)"
    openssl req -x509 -newkey rsa:2048 -sha256 -days 825 -nodes \
        -keyout "$APP/certs/server.key" -out "$APP/certs/server.crt" \
        -subj "/CN=$CN" \
        -addext "subjectAltName=DNS:$CN,DNS:localhost,IP:127.0.0.1" 2>/dev/null
    chown www-data:www-data "$APP/certs/server.key" "$APP/certs/server.crt"
fi

# 2) .env from environment (defaults target the compose stack)
cat > "$APP/.env" <<EOF
API_AUTH_DB=${API_AUTH_DB:-/app/data/auth.sqlite}
RE7_DB_HOST=${RE7_DB_HOST:-re7-db}
RE7_DB_PORT=${RE7_DB_PORT:-5432}
RE7_DB_NAME=${RE7_DB_NAME:-rate_engine}
RE7_DB_USER=${RE7_DB_USER:-re_admin}
RE7_DB_PASS=${RE7_DB_PASS:-_cfg.access}
JWT_SECRET=${JWT_SECRET:-$(openssl rand -base64 32)}
JWT_ALG=HS256
JWT_ACCESS_TTL=${JWT_ACCESS_TTL:-900}
JWT_REFRESH_TTL=${JWT_REFRESH_TTL:-28800}
EOF
chown www-data:www-data "$APP/.env"; chmod 600 "$APP/.env"

# 3) auth store + admin seed (run as www-data so the SQLite file stays writable)
su -s /bin/sh -c "php $APP/bin/re7-api-user.php init" www-data
if [ -n "${API_ADMIN_USER:-}" ] && [ -n "${API_ADMIN_PASS:-}" ]; then
    su -s /bin/sh -c "printf '%s\n' '$API_ADMIN_PASS' | php $APP/bin/re7-api-user.php add '$API_ADMIN_USER' --role admin" www-data 2>/dev/null \
        || echo "[re7-api] admin user already exists (seed skipped)"
fi

echo "[re7-api] starting php-fpm + nginx on :8443"
php-fpm -D
exec nginx -g 'daemon off;'
