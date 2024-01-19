# Use the official PostgreSQL image from Docker Hub
FROM postgres:15.1

# Environment variables to set up the database
ENV POSTGRES_DB=rate_engine
ENV POSTGRES_USER=re_admin
ENV POSTGRES_PASSWORD=_cfg.access

# Expose the default PostgreSQL port (5432)
EXPOSE 5432

# Copy custom initialization scripts (optional)
# COPY ./init.sql /docker-entrypoint-initdb.d/

# You can also mount a volume for persistent data
#VOLUME /var/lib/postgresql/data

# CMD instruction to start the PostgreSQL server
CMD ["postgres"]

