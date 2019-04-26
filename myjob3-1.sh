#!/bin/bash

./waf

temp=0
dirin="my-simulations/1_throughput/input/"
dirout="my-simulations/1_throughput/output/olsr-udp/"
rrstart=10001
rrend=10005

for ((aa=10; aa<=25; aa=aa+1))
do
  for ((dd=5; dd<=6; dd=dd+1))
  do
    for ((gg=1; gg<=3; gg=gg+1))
    do
      for ((rr=${rrstart}; rr<=${rrend}; rr=rr+1))
      do
        if [ ! -f "${dirout}mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt" ]; then
          echo mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt
          ./waf --run "mesh-loc-2 --apNum=${aa} --clNum=0 --aptx=true --rndSeed=${rr} -totalTime=40 --locationFile=${dirin}location_400_0_${aa}_${dd}.txt --gateways=${gg} --route=olsr" &> "${dirout}mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt"
        fi
      done
    done
  done
done
