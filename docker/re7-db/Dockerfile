FROM postgres:15.1

ENV POSTGRES_DB=rate_engine
ENV POSTGRES_USER=re_admin
ENV POSTGRES_PASSWORD=_cfg.access

EXPOSE 5432

# COPY ./init.sql /docker-entrypoint-initdb.d/

CMD ["postgres"]
