FROM ubuntu:jammy

ENV PATH="/dev-praas/bin:${PATH}"

RUN apt-get update && apt-get install -y --no-install-recommends valgrind libcurl4 ca-certificates\
  && update-ca-certificates

ENTRYPOINT valgrind /dev-praas/bin/local_worker --processes ${NUMPROCS} -v --hole-puncher-addr ${HOLEPUNCHER_ADDR}
