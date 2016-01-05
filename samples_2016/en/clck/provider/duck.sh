#!/bin/bash

# Generate a random number between 0 and 5
count=$(($RANDOM % 6))

if [ $count == 0 ]; then
    echo "honk"
else
    for i in $(seq 1 $count); do
        echo -n "quack "
    done
    echo # newline
fi
