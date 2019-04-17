#!/bin/bash

echo "only ap"

for ((rr=1; rr<=20; rr=rr+1))
do
  for ((aa=40; aa<=80; aa=aa+20))
  do
    for ((an=2; an<=5; an=an+1))
    do
      echo ${an} ${aa} ${rr}
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=${an} --clNum=0 --apStep=${aa} --totalTime=60 --startTime=1 --monitorInterval=0.5 --printRoutes=false --aptx=true" --run=mesh-aodv-2 &> output-aodv/aodv.result.${an}.${aa}.${rr}.1
    done
  done
done

echo "only cl"

for ((rr=1; rr<=20; rr=rr+1))
do
  for ((aa=40; aa<=80; aa=aa+20))
  do
    for ((an=2; an<=5; an=an+1))
    do
      echo ${an} ${aa} ${rr}
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=${an} --clNum=1 --apStep=${aa} --totalTime=60 --startTime=1 --monitorInterval=0.5 --printRoutes=false --aptx=false" --run=mesh-aodv-2 &> output-aodv/aodv.result.${an}.${aa}.${rr}.2
    done
  done
done

echo "ap + cl"

for ((rr=1; rr<=20; rr=rr+1))
do
  for ((aa=40; aa<=80; aa=aa+20))
  do
    for ((an=2; an<=5; an=an+1))
    do
      echo ${an} ${aa} ${rr}
      ./waf --command-template="%s --rndSeed=${rr} --gridSize=${an} --clNum=1 --apStep=${aa} --totalTime=60 --startTime=1 --monitorInterval=0.5 --printRoutes=false --aptx=true" --run=mesh-aodv-2 &> output-aodv/aodv.result.${an}.${aa}.${rr}.3
    done
  done
done
