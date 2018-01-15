#!/bin/bash

set -e

sudo apt-get update
sudo apt-get install  build-essential gcc-5 g++-5 make cmake g++ gcc-6 -y

sudo add-apt-repository ppa:ubuntu-toolchain-r/test

sudo apt update
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 1 --slave /usr/bin/g++ g++ /usr/bin/g++-5

sudo bash -c "apt-get upgrade; apt-get full-upgrade; apt-get install gcc-7 g++-7; update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7; update-alternatives --config gcc"

rm -rf cmake-3.10.1*
wget https://cmake.org/files/v3.10/cmake-3.10.1.tar.gz
tar -xvf cmake-3.10.1.tar.gz 
cd ./cmake-3.10.1 && ./bootstrap && make && make install

sudo update-alternatives --install /usr/bin/cmake cmake /usr/local/bin/cmake 1 --force

sudo apt install -y libssl-dev libhwloc-dev

