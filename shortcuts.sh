#!/bin/bash

build() {
    echo "Making sure you have the dependencies"
    DOES_CMAKE_EXIST=$(which cmake)
    if [ $DOES_CMAKE_EXIST != "" ]; then
        echo "Great, you have cmake. Now building..."
        cmake -S . -B build
        cmake --build build
    fi
}

if [ $1 = "" ]; then
    echo "Please type either: build, clean, run, quick."
elif [ $1 = "build" ]; then
    build
elif [ $1 = "clean" ]; then
    echo "Cleaning up..."
    rm -rf build
elif [ $1 = "run" ]; then
    chmod +x build/MotifDesigner
    ./build/MotifDesigner
elif [ $1 = "quick" ]; then
    rm -rf build/
    build
    chmod +x build/MotifDesigner
    ./build/MotifDesigner
fi
