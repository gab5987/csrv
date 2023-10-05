# #!/bin/bash

BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf $BUILD_DIR/*
else
    mkdir -p $BUILD_DIR
fi
cd $BUILD_DIR
cmake ..
make
# sudo make install
# make install PREFIX=$HOME/local

