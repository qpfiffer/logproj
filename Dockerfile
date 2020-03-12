FROM alpine:latest

RUN apk add --no-cache wget\
    git\
    make\
    automake\
    openssl-dev\
    libtool\
    autoconf\
    gcc\
    libc-dev\
    postgresql-dev\
    jansson-dev
RUN (cd /opt && git clone --depth 1 https://github.com/qpfiffer/38-Moths.git && \
     cd 38-Moths && make && make install)
RUN (cd /opt && wget https://github.com/technion/libscrypt/archive/v1.21.tar.gz && \
     tar -xf v1.21.tar.gz && cd libscrypt-1.21 && make && make install)
RUN (cd /opt && wget https://github.com/benmcollins/libjwt/archive/v1.12.0.tar.gz && \
     tar -xf v1.12.0.tar.gz && cd libjwt-1.12.0 && autoreconf -i && ./configure && \
     make && make install)
RUN (mkdir -p /opt/mon && cd /opt/mon && wget https://github.com/tj/mon/archive/master.tar.gz && tar -xf ./master.tar.gz && \
     cd mon-master && make install)

RUN mkdir -p /app
COPY src /app/src
COPY rsa256.key /app/rsa256.key
COPY rsa256.key.pub /app/rsa256.key.pub
COPY include /app/include
COPY templates /app/templates
COPY static /app/static
COPY Makefile /app/Makefile

RUN cd /app; make
COPY run/logproj.sh /etc/service/logproj/run

EXPOSE 8666
CMD ["/etc/service/logproj/run"]
