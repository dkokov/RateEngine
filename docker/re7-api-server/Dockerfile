# RateEngine7 API server container (nginx + php-fpm) — provisioning REST API.
# author: Dimitar Kokov
#
# Build context MUST be the repo root (see docker-compose.yml: context ../) so
# both the API app and the shared PHP lib are available:
#   /app        <- src/scripts/api-server
#   /lib/php    <- src/scripts/lib/php   (composer.json path repo "../lib/php")

FROM php:8.3-fpm-bookworm

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Sofia

RUN apt-get update && \
    apt-get install -y nginx libpq-dev libsqlite3-dev openssl ca-certificates unzip git && \
    docker-php-ext-install pdo pdo_pgsql pdo_sqlite && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

COPY --from=composer:2 /usr/bin/composer /usr/bin/composer

# shared lib first so the path repo ("../lib/php" -> /lib/php) resolves
COPY src/scripts/lib/php   /lib/php
COPY src/scripts/api-server /app

WORKDIR /app
# drop anything copied from the dev tree (root-owned vendor, dev .env, sqlite),
# then install fresh; the path dep symlinks to /lib/php which exists in-image.
RUN rm -rf vendor composer.lock .env data/* && \
    composer install --no-dev --no-interaction --no-progress

COPY docker/re7-api-server/nginx.conf              /etc/nginx/conf.d/re7-api.conf
COPY docker/re7-api-server/run-re7-api-server.sh   /run.sh
RUN rm -f /etc/nginx/sites-enabled/default 2>/dev/null || true; \
    chmod +x /run.sh; \
    mkdir -p /app/data /app/certs; \
    chown -R www-data:www-data /app/data /app/certs

EXPOSE 8443
ENTRYPOINT ["/run.sh"]
