FROM ubuntu:jammy AS builder

ENV DEBIAN_FRONTEND=noninteractive 

RUN apt-get update\
  && apt-get install --reinstall -y --no-install-recommends apt-transport-https ca-certificates\
  && apt-get install -y --no-install-recommends git cmake build-essential libboost-all-dev libssl-dev\
  && update-ca-certificates

