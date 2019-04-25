#!/bin/bash

temp=0

for ((rr=10001; rr<=10020; rr=rr+1))
do
  for ((aa=11; aa<=25; aa=aa+2))
  do
    for ((dd=1; dd<=10; dd=dd+1))
    do
      for ((gg=1; gg<=3; gg=gg+1))
      do
        if [ ${aa} = 11 ] && [ ${dd} = 1 ] && [ ${gg} = 1 ] && [ ${rr} = 10001 ]; then
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
