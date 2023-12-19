#!/bin/bash
set +e
docker stop redis || true
docker rm redis || true
docker run --net=host --name redis --rm -d redis
/bin/bash $(dirname $0)/integration/docker_ssl/run.sh
/bin/bash $(dirname $0)/integration/docker_expressjs/run.sh
