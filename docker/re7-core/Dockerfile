# RateEngine7 Docker container
# author: Dimitar Kokov
# version: 0.2
# date: 2026-06-11

FROM debian:12.2

ENV DEBIAN_FRONTEND noninteractive
ENV NAME re7-core

# dependencies
RUN apt-get update && \
apt-get install -y gcc make sudo apt-utils tzdata net-tools \
libxml2-dev libjson-c-dev libpq-dev postgresql-client \
mc htop && \
apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /re7-core
COPY . .
COPY docker/re7-core/run-re7-core.sh .
COPY docker/re7-core/.pgpass /root/.pgpass.tpl

RUN chmod +x /re7-core/run-re7-core.sh

# build core + modules + install
RUN cd /re7-core/src/ && \
make && make modules && make install PREFIX=/re7-core/tmp/

EXPOSE 9090/tcp

ENTRYPOINT ["./run-re7-core.sh"]
