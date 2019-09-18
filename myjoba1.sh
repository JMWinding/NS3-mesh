#!/bin/bash

./waf

for ((rrstart=$1; rrstart<=$2; rrstart=rrstart+1))
do
  echo $rrstart
  bash myjob-aodv-udp-error.sh ${rrstart} &
done
