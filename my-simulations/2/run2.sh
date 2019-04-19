#!/bin/bash

echo "only cl"

for ((rr=1; rr<=50; rr=rr+1))
do
  for ((ss=1.8; ss<=2.2; ss=ss+0.1))
  do
    for ((dd=1; dd<=5; dd=dd+1))
    do
      echo ${ss} ${dd} ${rr}
      ./waf --run 'mesh-aodv-3 --rndSeed=${rr} --apNum=27 --aptx=true --gateways=3 --datarate=${dd}Mbps --locationFile=location.txt --scale=${ss}' &> my-simulations/2/aodv.result.${ss}.${dd}.${rr}
    done
  done
done