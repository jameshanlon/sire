#!/bin/bash

# Exit on reading uninitialised variable
#set -u 
# Exit if any statement fails
set -e
#set -x

if [ -z "$1" ] ; then SRC_FILE="test.x" ; else SRC_FILE=$1 ; fi

NUM_CORES=8

RUNTIME_DIR="runtime"
INC_DIR="include"
PROG_OUT="program.S"
JUMPTAB_OUT="jumpTable.S"
CP_OUT="cp.S"
TARGET="configs/XMP-${NUM_CORES}.xn"
SYS_DEF="platform.h"

CMP_ARGS="$TARGET" #"-fverbose-asm"
LINK_ARGS="-nostdlib -Xmapper --nochaninit"

RUNTIME_FILES="
system.xc
host.xc
guest.xc
util.xc
master.xc
host_.S
system_.S
master_.S
slave.S
slaveJumpTable.S"

echo "Building a $NUM_CORES core executable for $TARGET"

echo "Creating headers"
echo "#define NUM_CORES $NUM_CORES" > $INC_DIR/$SYS_DEF
echo "#define DIMENSIONS $(echo "l($NUM_CORES)/l(2)" | bc -l | xargs printf "%1.0f")" \
    >> $INC_DIR/$SYS_DEF

#set -x

# Program object
echo "Compiling program"
bin/sire $SRC_FILE
xcc -c $PROG_OUT    -o program.o
xcc -c $JUMPTAB_OUT -o jumpTable.o
xcc -c $CP_OUT      -o cp.o

# Runtime objects
echo "Compiling runtime system"
for f in $RUNTIME_FILES
do xcc $CMP_ARGS -O2 -c $RUNTIME_DIR/$f 
done

# Master
echo "Linking master XE"
xcc $CMP_ARGS $LINK_ARGS -first jumpTable.o -first cp.o \
    system_.o system.o \
    guest.o host.o host_.o util.o \
    master.o master_.o program.o \
    -o master.xe

# Slave (link blank jump table and the program cp)
echo "Linking slave XE"
xcc $CMP_ARGS $LINK_ARGS -first slaveJumpTable.o -first cp.o \
    system_.o system.o \
    guest.o host.o host_.o util.o slave.o \
    -o slave.xe

# Split the slave binary
echo "Splitting slave"
xobjdump --split slave.xe >/dev/null

# Replace slave images
echo -e "\rReplacing node 0, core\c"
for(( i=1; i<$NUM_CORES; i++ ))
do  node=$(($i/4))
    core=$(($i%4))
    if [ "$core" -eq "0" ]
    then 
        echo -e "\r                                \c"
        echo -e "\rReplacing node $node, core 0\c"
    else
        echo -e " $core\c"
    fi
    xobjdump master.xe -r $node,$core,image_n0c0.elf >/dev/null
done
echo ""

# Cleanup
echo "Cleaning up"
mv master.xe out.xe
rm slave.xe
rm config.xml platform_def.xn program_info.txt image_n0c0.elf
rm *.o *.S
