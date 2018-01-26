#!/usr/bin/env bash

set -e 


export SYSTEM_NPROC=`../scripts/count-nproc`;
export SYSTEM_CACHE_L2=`../scripts/count-cache-l2`;
export SYSTEM_CACHE_L3=`../scripts/count-cache-l3`;

echo SYSTEM_NPROC=$SYSTEM_NPROC;
echo SYSTEM_CACHE_L2=$SYSTEM_CACHE_L2;
echo SYSTEM_CACHE_L3=$SYSTEM_CACHE_L3;

rm -rf ./CMakeFiles

cmake -DCONFIG_SYSTEM_NPROC=$SYSTEM_NPROC -DCONFIG_SYSTEM_CACHE_L2=$SYSTEM_CACHE_L2 -DCONFIG_SYSTEM_CACHE_L3=$SYSTEM_CACHE_L3 . && make
