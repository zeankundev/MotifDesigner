#!/bin/bash

build() {
    echo "Making sure you have the dependencies"
    DOES_CMAKE_EXIST=$(which cmake)
    if [ $DOES_CMAKE_EXIST != ""]; then
        echo "Great, you have cmake. Now building..."

    fi
}

if [ $1 = ""]; then
    echo "Please type either: build, clean, quick."
