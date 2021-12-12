FROM praas/cpp_builder AS builder

RUN apt-get install -y --no-install-recommends libcurl4-openssl-dev libssl-dev uuid-dev zlib1g-dev libcurl4 ca-certificates\
  && update-ca-certificates

WORKDIR /build/
RUN mkdir /source && cd /source && git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp
RUN cd /build && cmake -B . -S /source/aws-sdk-cpp -DBUILD_ONLY="s3" -DENABLE_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/opt/
RUN cmake --build . --parallel 4
RUN cmake --install .

FROM ubuntu:jammy

COPY --from=builder /opt /opt

