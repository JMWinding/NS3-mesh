#!/bin/bash

temp=0
rrstart=10002

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=11; aa<=25; aa=aa+2))
  do
    for ((dd=1; dd<=10; dd=dd+1))
    do
      for ((gg=1; gg<=3; gg=gg+1))
      do
        if [ ${aa} = 17 ] && [ ${dd} = 8 ] && [ ${gg} = 1 ] && [ ${rr} = ${rrstart} ]; then
          temp=1
        fi
        if [ ${temp} = 1 ]; then
          echo mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt
          ./waf --run "mesh-loc-1 --apNum=${aa} --clNum=0 --aptx=true --rndSeed=${rr} -totalTime=40 --locationFile=my-simulations/1_location_and_throughput/input/location_400_0_${aa}_${dd}.txt --gateways=${gg} --scale=0.8" &> my-simulations/1_location_and_throughput/output/mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt
        fi
      done
    done
  done
done
