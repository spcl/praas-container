FROM praas/aws-sdk-s3 AS aws
FROM praas/cpp_builder AS builder

ARG GITHUB_ACCESS_TOKEN

COPY --from=aws /opt /opt

RUN apt-get install -y --no-install-recommends libhiredis0.14 libhiredis-dev libboost-dev libboost-system-dev libboost-thread-dev libssl-dev

WORKDIR /build/
ADD . /source/
RUN cmake -DCMAKE_PREFIX_PATH=/opt/ -DCMAKE_BUILD_TYPE=Release -B . -S /source/
RUN cmake --build . -j4 --target process_exec

FROM ubuntu:jammy

WORKDIR /praas
ENV PATH="/dev-praas/bin:${PATH}"

RUN apt-get update && apt-get install -y --no-install-recommends libcurl4 ca-certificates\
  && update-ca-certificates
COPY --from=aws /opt /opt
COPY --from=builder /build /dev-praas

