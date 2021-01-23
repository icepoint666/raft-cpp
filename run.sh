#! /bin/sh
if [ "$1" = "mushroom" ]
then
    if [ -d "mushroom/build" ]; then
        rm -r mushroom/build
    fi
    mkdir mushroom/build
    cd mushroom/build
    cmake ..
    make
elif [ "$1" = "test" ]
then
    if [ -d "build" ]; then
        rm -r build
    fi
    mkdir build
    cd build
    cmake ..
    make
fi


