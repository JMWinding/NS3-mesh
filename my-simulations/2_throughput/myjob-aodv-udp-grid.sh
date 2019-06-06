#!/bin/bash

./waf

temp=0
dirin="my-simulations/2_throughput/input/"
dirout="my-simulations/2_throughput/output/aodv-udp/"
rrstart=10001
rrend=10016

for ((rr=${rrstart}; rr<=${rrend}; rr=rr+1))
do
  for aa in 12 15 16 18 20 24 25
  do
    for ((gg=1; gg<=3; gg=gg+1))
    do
      if [ ! -f "${dirout}grid_400_0_${aa}_${gg}_${rr}.txt" ]; then
        echo grid_400_0_${aa}_${gg}_${rr}.txt
        ./waf --run "mesh-loc-1 --apNum=${aa} --clNum=0 --aptx=true --rndSeed=${rr} -totalTime=120 --locationFile=${dirin}grid_400_0_${aa}.txt --scale=80 --gateways=${gg}" &> "${dirout}grid_400_0_${aa}_${gg}_${rr}.txt"
      fi
    done
  done
done
