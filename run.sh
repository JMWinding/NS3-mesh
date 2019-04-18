#!/bin/bash

echo "only cl"

for ((rr=1; rr<=20; rr=rr+1))
do
  for ((aa=40; aa<=60; aa=aa+5))
  do
    for ((an=3; an<=7; an=an+2))
    do
      echo ${an} ${aa} ${rr}
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=${an} --clNum=4 --apStep=${aa} --clStep=20 --totalTime=60 --startTime=1 --monitorInterval=0.5 --printRoutes=false --aptx=false --datarate=5Mbps" --run=mesh-aodv-2 &> output-aodv/aodv.result.${an}.${aa}.${rr}.2
    done
  done
done
