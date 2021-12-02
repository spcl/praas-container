FROM praas/cpp_builder AS builder

WORKDIR /build/
ADD . /source/
RUN cmake -B . -S /source/
RUN cmake --build . --target control_plane

FROM ubuntu:jammy

WORKDIR /praas
COPY --from=builder /build/bin/control_plane /praas/control_plane 

ENTRYPOINT ["/praas/control_plane"]
