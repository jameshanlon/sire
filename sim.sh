#!/bin/bash

# Exit on reading uninitialised variable
set -u 
# Exit if any statement fails
#set -e

#bin/cmp simple.x -o out.o
#sh build.sh XC-1 4 out.o 

echo "Running simulation..."
xsim -t --iss --warn-resources --max-cycles 100000 out.xe > dump.txt

#if [ "$1" = "-r" ] 
#then 
#java -Xmx1024m -cp tracer Tracer $NUM_CORES dump.txt &
#echo "Loading trace..."
java -Xmx1024m -cp tracer TraceView 64 dump.txt &
#fi
