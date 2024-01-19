# RateEngine6 server Docker container
# author: Dimitar Kokov
# version: 0.1
# date: 2024-01-19

# docker image build -t re6-core .
#

FROM debian:12.2

ENV DEBIAN_FRONTEND noninteractive
ENV NAME re6-core

#INSTALL DEPENDENCIES
RUN apt-get update && \
apt-get install -y wget gcc make git autoconf libtool automake sudo apt-utils tzdata net-tools \
libxml2-dev libpq-dev postgresql-client \
mc htop cmake

WORKDIR /re6-core
#ADD . /re6-core
COPY . .
COPY docker/re6-core/* .

RUN chmod +x /re6-core/run-re6-core.sh

RUN cd /re6-core/src/ && \
make && make install PREFIX=/re6-core/tmp/

EXPOSE 9090/tcp

ENTRYPOINT ["./run-re6-core.sh"]
#CMD ["./run-re6-core.sh"]

