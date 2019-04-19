#!/bin/bash

echo "only cl"

for ((rr=1; rr<=50; rr=rr+1))
do
  for ((ss=180; ss<=220; ss=ss+10))
  do
    for ((dd=1; dd<=5; dd=dd+1))
    do
      echo ${ss} ${dd} ${rr}
      ./waf --command-template="%s --rndSeed=${rr} --apNum=27 --aptx=true --gateways=3 --datarate=${dd}Mbps --locationFile=location.txt --scale=${ss} --totalTime=60" --run=mesh-aodv-3 &> my-simulations/2/aodv.result.${ss}.${dd}.${rr}
    done
  done
done
