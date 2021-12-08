FROM ubuntu:jammy

RUN apt-get update && apt-get install -y --no-install-recommends libhiredis0.14 openssl valgrind libboost-system-dev libboost-thread-dev

WORKDIR /dev-praas/bin/

ENTRYPOINT valgrind /dev-praas/bin/control_plane -a ${IP_ADDRESS} -r ${REDIS_HOST} --backend local --local-server ${LOCAL_WORKER} -v --https-port ${HTTP_PORT} --ssl-server-cert /usr/local/etc/praas/server.pem --ssl-server-key /usr/local/etc/praas/server.key
