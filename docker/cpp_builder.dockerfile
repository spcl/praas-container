FROM ubuntu:jammy AS builder

RUN apt-get update\
  && apt-get install --reinstall -y --no-install-recommends apt-transport-https ca-certificates\
  && apt-get install -y --no-install-recommends git cmake build-essential\
  && update-ca-certificates

