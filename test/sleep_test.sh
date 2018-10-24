#!/bin/sh

START=`date +%s%N`;
sleep 0.5
END=`date +%s%N`;
time=$((END-START))
time=`expr $time/1000000`
echo $time


