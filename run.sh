#!/bin/bash

for ((rr=10000; rr<=10010; rr=rr+1))
do
  for ((aa=50; aa<=300; aa=aa+50))
  do
    for ((an=2; an<=5; an=an+1))
    do
      echo ${an} ${aa} ${rr}
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=${an} --clNum=0 --apStep=${aa} --totalTime=1000 --startTime=100 --monitorTime=400 --printRoutes=false --aptx=true" --run=mesh-aodv-3 &> output-aodv/aodv.result.${an}.${aa}.${rr}
    done
  done
done
