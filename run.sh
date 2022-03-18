#!/bin/sh

#sudo chown -R jamie /dev/bus/usb/*
#xrun $1 $2 $3 $4

echo -e "Loading binary...\c"
xrun out.xe
echo "done"

sleep 5
echo -e "Dumping state...\c"
xrun --dump-state > dump.txt
echo "done"

# Grab the value of r11 from core 0 and convert it to ms (from ns)
T=$(cat dump.txt | grep "r11 " | tail -1)
T=$(echo $T | cut -d ' ' -f 3)
echo $(($T))
