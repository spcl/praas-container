# PraaS Container

This repository holds PraaS container runtime and system components.
In future versions, we're going to split the system components into a separate repositoryt .

# Installations

We require CMake >= 3.18 and a C++17-compatible compiler.
Currently, we need a GitHub access token to access the private repository
hosting `tcpunch` library.

```
GITHUB_ACCESS_TOKEN="YOUR_TOKEN" cmake -B <your-build-dir> -S .
GITHUB_ACCESS_TOKEN="YOUR_TOKEN" cmake --build build -j4
```

# System

PraaS consists of three major components - container runtime, control plane, and local worker.

### Runtime

The C++ runtime defines the subprocess logic and a session executable.
The subprocess forks new processes to launch sessions.

### Control Plane

Deploys control plane responsible for handling user requests (HTTPS frontend) and allocates
processes and sessions (TCP backend).
It deploys with `haproxy` load balancer, `redis` cache and `tcpunch` hole puncher.

### Local Worker

The worker allocates new subprocesses and sessions.
There are two version: simplified version deployed as Docker container. There, we run
each subprocess as a thread in the main process, and fork sessions as subprocesses.
It is used primarily for development.

The main deployment version runs as a process and deploys new Docker containers running
subprocesses.

# Deployment

We support two types of deployments - local development version and complete deployment.

The local deployment maps local build directory on the containers:

```
DEV=build docker-compose -f docker-compose-dev.yml up
```

The standard deployment uses prebuilt containers:

```
docker-compose -f docker-compose.yml up
```

TODO: production deployment

## Docker

Build standard and development containers:

```
docker build -f docker/cpp_builder.dockerfile . -t praas/cpp_builder

docker build -f docker/control_plane.dockerfile . -t praas/control_plane
docker build -f docker/control_plane-dev.dockerfile . -t praas/control_plane-dev

docker build -f docker/local_worker.dockerfile . -t praas/local_worker
docker build -f docker/local_worker-dev.dockerfile . -t praas/local_worker-dev
```

### OpenSSL

We deploy an HTTPS frontend for PraaS control plane with a self-signed certificate.

Install `trustme` Python package and run:

```cli
python -m trustme -i localhost 127.0.0.1 ::1 <your-server-address>
```

This will generate `server.pem`, `server.key` and `client.pem`. Use `server.pem` and `server.key`
for an instance of control plane, and use `client.pem` in the client library.

```
SECRET_CERT=server.pem SECRET_KEY=server.key CLIENT_CERT=client.pem DEV=build docker-compose -f docker-compose-dev.yml up
```
