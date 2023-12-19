#!/bin/bash
cd $(dirname $0)

mkdir -p /tmp/nginx
docker stop nginx-ssl || true
docker rm nginx-ssl | true
docker run --name nginx-ssl --rm -d -ti \
  -p 8081:80 \
  -p 8080:8080 \
  -p 8443:443 \
  -e SSLKEYLOGFILE=/tmp/ssl.log \
  -v /tmp/nginx:/tmp \
  -v $PWD/nginxconfig:/etc/nginx \
  -v $PWD/nginx_data:/var/www/example.com \
  nginx
