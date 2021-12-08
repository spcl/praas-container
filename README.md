# praas-container




## OpenSSL

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
