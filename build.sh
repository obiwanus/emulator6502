#!/bin/bash

echo "Starting build"

CFLAGS="-g -std=c++11 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1"
LFLAGS="$(pkg-config --cflags --libs x11) -ldl"

gcc $CFLAGS ../vm/linux_layer.cpp $LFLAGS -o os