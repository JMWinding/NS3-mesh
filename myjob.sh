#!/bin/bash

for ((rrstart=$1; rrstart<=$2; rrstart=rrstart+1))
do
  gnome-terminal --tab -- sh -c "bash myjob-aodv-udp-ideal.sh ${rrstart}"
  sleep 5
done
