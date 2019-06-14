#!/bin/bash

./waf

temp=0

simtip='1_throughput_rate_adaptation'
route='aodv'
app='udp'
ratecontrol='ideal'

dirin="my-simulations/${simtip}/input"
dirout="my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}"
[ ! -d ${dirout} ] && mkdir ${dirout}

rrstart=10001
rrend=10016

for ((rr=${rrstart}; rr<=${rrend}; rr=rr+4))
do
  for ((aa=10; aa<=25; aa=aa+1))
  do
    for ((dd=1; dd<=10; dd=dd+1))
    do
      for ((gg=1; gg<=3; gg=gg+1))
      do
        if [ ! -f "${dirout}/mesh_400_0_${aa}_${dd}_${gg}_${rr}.xmp" ]; then
          echo mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt
          ./waf --run "mesh-loc-1 --apNum=${aa} --aptx=true --gateways=${gg} --gatx=true --rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=80 --locationFile=${dirin}/location_400_0_${aa}_${dd}.txt --flowout=${dirout}/mesh_400_0_${aa}_${dd}_${gg}_${rr}.xmp --scale=80" &> "${dirout}/mesh_400_0_${aa}_${dd}_${gg}_${rr}.txt"
        fi
      done
    done
  done
done
