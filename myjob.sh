#!/bin/bash

for ((rrstart=10001; rrstart<=10016; rrstart=rrstart+1))
do
  gnome-terminal --tab -- bash myjob-aodv-udp-ideal.sh ${rrstart}
  sleep 10
done

