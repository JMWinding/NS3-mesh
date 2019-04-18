#!/bin/bash

echo "only cl"

for ((rr=1; rr<=20; rr=rr+1))
do
  for ((aa=40; aa<=60; aa=aa+5))
  do
    for ((dd=1; dd<=5; dd=dd+1))
    do
      an=3
      echo ${an} ${aa} ${rr} ${dd}
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=${an} --gateway=4 --clNum=0 --apStep=${aa} --clStep=20 --totalTime=60 --startTime=1 --monitorInterval=0.5 --printRoutes=false --aptx=true --datarate=${dd}Mbps" --run=mesh-aodv-2 &> result/1/aodv.result.${an}.${aa}.${rr}.${dd}
      an=4
      echo ${an} ${aa} ${rr} ${dd}
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=${an} --gateway=5 --clNum=0 --apStep=${aa} --clStep=20 --totalTime=60 --startTime=1 --monitorInterval=0.5 --printRoutes=false --aptx=true --datarate=${dd}Mbps" --run=mesh-aodv-2 &> result/1/aodv.result.${an}.${aa}.${rr}.${dd}
      an=5
      echo ${an} ${aa} ${rr} ${dd}
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=${an} --gateway=12 --clNum=0 --apStep=${aa} --clStep=20 --totalTime=60 --startTime=1 --monitorInterval=0.5 --printRoutes=false --aptx=true --datarate=${dd}Mbps" --run=mesh-aodv-2 &> result/1/aodv.result.${an}.${aa}.${rr}.${dd}
    done
  done
done
