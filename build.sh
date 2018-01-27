#!/usr/bin/env bash

set -e

rm -rf ./CMakeFiles ./CMakeCache.txt

bash -c "cd ./sys_auto_adjust && ./build.sh"

# export CXX=/usr/bin/g++-7
#  -DCMAKE_CXX_COMPILER=/usr/bin/g++-7

. .env
cmake \
 -DCMAKE_RULE_MESSAGES:BOOL=OFF \
 -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
 -DCMAKE_BUILD_TYPE=Release \
  \
 .
 
make clean && make

# sudo systemctl restart xmr-slim
# sleep 5
# sudo systemctl status xmr-slim


