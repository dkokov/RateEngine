# RateEngine6 server Docker container
# Dimitar Kokov
# 2024-01-16

# docker image build .
# docker image build -t sherpa-onnx-sr .
#

FROM debian:12.2

ENV DEBIAN_FRONTEND noninteractive
ENV NAME sherpa-onnx

WORKDIR /re6-core
ADD . /re6-core
COPY . .

RUN chmod +x /re6-core/run-re6-core.sh

#INSTALL DEPENDENCIES
RUN apt-get update && \
apt-get install -y wget gcc make git autoconf libtool automake sudo apt-utils tzdata net-tools \
apt-get install libxml2-dev libpq-dev \
mc htop cmake

RUN cd /re6-core
#RUN git clone https://github.com/k2-fsa/sherpa-onnx && \
RUN cd sherpa-onnx && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j8

ENTRYPOINT ["./run-re6-core.sh"]

