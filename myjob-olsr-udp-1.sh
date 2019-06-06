#!/bin/bash

./waf

temp=0
dirin="my-simulations/2_throughput/input/"
dirout="my-simulations/2_throughput/output/olsr-udp/"
rrstart=10001
rrend=10016

for ((rr=${rrstart}; rr<=${rrend}; rr=rr+16))
do
  for ((aa=10; aa<=25; aa=aa+1))
  do
    for ((dd=1; dd<=10; dd=dd+1))
    do
      for ((gg=1; gg<=3; gg=gg+1))
      do
        if [ ! -f "${dirout}mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt" ]; then
          echo mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt
          ./waf --run "mesh-loc-1 --apNum=${aa} --clNum=0 --aptx=true --rndSeed=${rr} -totalTime=80 --locationFile=${dirin}location_400_0_${aa}_${dd}.txt --scale=80 --gateways=${gg} --route=olsr" &> "${dirout}mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt"
        fi
      done
    done
  done
done
