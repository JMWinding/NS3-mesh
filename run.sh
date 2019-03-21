#!/bin/bash

for ((rr=1; rr<=10; rr=rr+1))
do
  for ((aa=100; aa<=300; aa=aa+50))
  do
    for ((cc=1; cc<=10; cc=cc+2))
    do
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=3 --apNum=9 --clNum=${cc} --apStep=${aa} --totalTime=50" --run=mesh-aodv-3 &> output-aodv/aodv.result.${aa}.${cc}.${rr}
    done
  done
done
