FROM ubuntu:jammy AS builder

RUN apt-get update && apt-get install -y --no-install-recommends cmake build-essential

