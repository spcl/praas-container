#!/bin/bash

docker build -f docker/cpp_builder.dockerfile . -t praas/cpp_builder
docker build -f docker/control_plane.dockerfile . -t praas/control_plane

