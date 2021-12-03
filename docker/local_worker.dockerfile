FROM praas/cpp_builder AS builder

WORKDIR /build/
ADD . /source/
RUN cmake -B . -S /source/
RUN cmake --build . --target local_worker

FROM ubuntu:jammy

WORKDIR /praas
COPY --from=builder /build/bin/local_worker /praas/local_worker 

ENTRYPOINT /praas/local_worker --processes ${NUMPROCS}
