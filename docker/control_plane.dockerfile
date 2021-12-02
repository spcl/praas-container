FROM praas/cpp_builder AS builder

RUN apt-get install -y --no-install-recommends libhiredis0.14 libhiredis-dev

WORKDIR /build/
ADD . /source/
RUN cmake -B . -S /source/
RUN cmake --build . --target control_plane

FROM ubuntu:jammy

WORKDIR /praas
RUN apt-get update && apt-get install -y --no-install-recommends libhiredis0.14
COPY --from=builder /build/bin/control_plane /praas/control_plane 

ENTRYPOINT /praas/control_plane -r ${REDIS_HOST}
