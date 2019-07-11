#!/bin/bash

./waf

for ((rrstart=$1; rrstart<=$2; rrstart=rrstart+1))
do
  bash myjob-aodv-udp-error.sh ${rrstart} &
done
