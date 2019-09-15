#!/bin/bash

./waf

temp=0
dirin="my-simulations/3_real_topology/input/"
dirout="my-simulations/3_real_topology/output/"
rrstart=10001
rrend=10100

for ((rr=${rrstart}; rr<=${rrend}; rr=rr+1))
do
  route="aodv"
  app="udp"
  if [ ! -f "${dirout}mesh_26_3_${rr}_${route}_${app}.txt" ]; then
    echo mesh_26_3_${rr}.txt
    ./waf --run "mesh-loc-2 --apNum=26 --clNum=0 --aptx=true --rndSeed=${rr} -totalTime=120 --locationFile=${dirin}location2.txt --gateways=3 --route=${route} --app=${app} -datarate=5e5" &> "${dirout}mesh_26_3_${rr}_${route}_${app}.txt"
  fi
done
