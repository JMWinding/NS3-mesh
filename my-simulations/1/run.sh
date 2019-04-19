#!/bin/bash

for ((rr=1; rr<=20; rr=rr+1))
do
  for ((aa=40; aa<=60; aa=aa+5))
  do
    for ((dd=1; dd<=10; dd=dd+1))
    do
      for ((an=3; an<=9; an=an+1))
      do
        echo ${an} ${aa} ${rr} ${dd}
        ./waf --command-template="%s --rndSeed=${rr} --gridSize=3 --apNum=${an} --gateway=0 --clNum=0 --apStep=${aa} --clStep=20 --totalTime=60 --startTime=1 --monitorInterval=1 --printRoutes=false --aptx=true --datarate=${dd}Mbps" --run=mesh-aodv-2 &> result/1/aodv.result.${an}.${aa}.${rr}.${dd}
      done
    done
  done
done
