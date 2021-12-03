FROM ubuntu:jammy

ENTRYPOINT /dev-praas/bin/local_worker --processes ${NUMPROCS} -v
