FROM ubuntu:jammy

RUN apt-get update && apt-get install -y --no-install-recommends libhiredis0.14

ENTRYPOINT /dev-praas/bin/control_plane -r ${REDIS_HOST} --backend local --local-server ${LOCAL_WORKER} -v
