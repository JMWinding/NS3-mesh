#!/bin/bash

./waf

temp=0
dirin="my-simulations/3_real_topology/input/"
dirout="my-simulations/3_real_topology/output/"
rrstart=10001
rrend=10050

for ((rr=${rrstart}; rr<=${rrend}; rr=rr+1))
do
  route="aodv"
  app="udp"
  if [ ! -f "${dirout}mesh_27_3_${rr}_${route}_${app}.txt" ]; then
    echo mesh_27_3_${rr}.txt
    ./waf --run "mesh-loc-3 --apNum=27 --clNum=0 --aptx=true --rndSeed=${rr} -totalTime=40 --locationFile=${dirin}location.txt --gateways=3 --route=${route} --app=${app}" &> "${dirout}mesh_27_3_${rr}_${route}_${app}.txt"
  fi

  route="aodv"
  app="tcp"
  if [ ! -f "${dirout}mesh_27_3_${rr}_${route}_${app}.txt" ]; then
    echo mesh_27_3_${rr}.txt
    ./waf --run "mesh-loc-3 --apNum=27 --clNum=0 --aptx=true --rndSeed=${rr} -totalTime=40 --locationFile=${dirin}location.txt --gateways=3 --route=${route} --app=${app}" &> "${dirout}mesh_27_3_${rr}_${route}_${app}.txt"
  fi

  route="olsr"
  app="udp"
  if [ ! -f "${dirout}mesh_27_3_${rr}_${route}_${app}.txt" ]; then
    echo mesh_27_3_${rr}.txt
    ./waf --run "mesh-loc-3 --apNum=27 --clNum=0 --aptx=true --rndSeed=${rr} -totalTime=40 --locationFile=${dirin}location.txt --gateways=3 --route=${route} --app=${app}" &> "${dirout}mesh_27_3_${rr}_${route}_${app}.txt"
  fi
done
