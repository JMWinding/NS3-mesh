#!/bin/bash

for ((rrstart=$1; rrstart<=$2; rrstart=rrstart+1))
do
  xterm -e "bash myjob-aodv-udp-ideal.sh ${rrstart}" &
done

wait
