FROM postgres:15.1

# POSTGRES_DB / POSTGRES_USER / POSTGRES_PASSWORD are provided by
# docker-compose (from docker/.env) — never bake the password into the image.

EXPOSE 5432

# COPY ./init.sql /docker-entrypoint-initdb.d/

CMD ["postgres"]
