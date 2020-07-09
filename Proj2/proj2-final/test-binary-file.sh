#! /bin/bash

PORT=$1

cat /dev/urandom | head -c 1000000 > flarge
curl -o download 127.0.0.1:${PORT}/flarge
diff download flarge