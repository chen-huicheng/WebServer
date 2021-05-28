#!/bin/bash

mkdir ./build
cd ./build
cmake ..
make clean
make
