#!/bin/bash

temp=0

simtip='1_throughput_rate_adaptation'
route='olsr'
app='tcp'
ratecontrol='ideal'
startTime=10

rrstart=$1

dirin="my-simulations/${simtip}/input"
dirout="my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}-${rrstart}"
[ ! -d ${dirout} ] && mkdir -p ${dirout}

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=10; aa<=25; aa=aa+1))
  do
    for ((dd=1; dd<=10; dd=dd+1))
    do
      for ((gg=1; gg<=3; gg=gg+1))
      do
        if [ ! -f "${dirout}/mesh_400_0_${aa}_${dd}_${gg}_${rr}.xmp" ]; then
          echo mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt
          ./waf --run "mesh-loc-1 --apNum=${aa} --aptx=true --app=${app} --gateways=${gg} --rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=120 --startTime=${startTime} --locationFile=${dirin}/location_400_0_${aa}_${dd}.txt --flowout=${dirout}/mesh_400_0_${aa}_${dd}_${gg}_${rr}.xmp --scale=80" &> "${dirout}/mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt"
        fi
      done
    done
  done
done
