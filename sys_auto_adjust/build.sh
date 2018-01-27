#!/usr/bin/env bash

set -e 

rm -f sys_auto_adjust.hpp

export SYSTEM_NPROC=`../scripts/count-nproc`;
export SYSTEM_CACHE_L2=`../scripts/count-cache-l2`;
export SYSTEM_CACHE_L3=`../scripts/count-cache-l3`;

rm -rf ./CMakeFiles ./CMakeCache.txt

cmake . && make

./bin/sys-auto-adjust | grep '#' > ../xmrstak/build_constants.hpp
./bin/sys-auto-adjust | grep 'export' > ../.env
