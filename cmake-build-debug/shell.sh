#!/bin/sh

while read line
do
    printf "%s\n" "$line" > input_pipe
done < "$1"
