#!/bin/bash

cmake -B ./build_linux -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cd build_linux

echo "--------- Start Compile ---------";
if [ -n "$1" ]; then
	cd samples/"$1"
	make "$1" 
	echo "--------- End Compile ---------";
	./"$1"
else
	cd samples/triangle
	make triangle 
	echo "--------- End Compile ---------";
	./triangle
fi
