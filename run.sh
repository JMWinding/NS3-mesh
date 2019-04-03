#!/bin/bash

for ((rr=1; rr<=10; rr=rr+1))
do
  for ((aa=100; aa<=300; aa=aa+50))
  do
    for ((an=3; an<=6; an=an+1))
    do
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=${an} --clNum=0 --apStep=${aa} --totalTime=50 -aptx=true" --run=mesh-aodv-3 &> output-aodv/aodv.result.${an}.${aa}.${rr}
    done
  done
done
