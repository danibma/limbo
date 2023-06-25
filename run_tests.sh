#!/bin/bash

cmake -B ./build_linux -G "Unix Makefiles"
cd build_linux

echo "--------- Start Compile ---------";
make limbo_tests
./limbo_tests -s
echo "--------- End Compile ---------";
