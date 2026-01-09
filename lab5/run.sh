#!/bin/bash


echo "port opening"
socat -d -d pty,raw,echo=0,link=/tmp/vcp0 pty,raw,echo=0,link=/tmp/vcp1 &

sleep 2

echo "start emulator"
./build/emulator /tmp/vcp0

echo "start listener"
./build/listener /tmp/vcp1