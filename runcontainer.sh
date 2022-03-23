#!/bin/sh
docker run --rm -v $(pwd):/home/sensconnect/src --network=host sensconnect src/sensconnect