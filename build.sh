#!/bin/bash
if [ "$1" = "-r" ] 
then
    echo "Building Release"
    cmake -DCMAKE_BUILD_TYPE=Release -Brelease -H.
    (cd release && make)
else
    echo "Building Debug"
    cmake -DCMAKE_BUILD_TYPE=Debug -Bdebug -H.
    (cd debug && make)
fi
