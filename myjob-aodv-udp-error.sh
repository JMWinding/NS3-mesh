#!/bin/bash

simtip='4_error'
route='aodv'
app='udp'
ratecontrol='const'

rrstart=$1

dirin="./my-simulations/${simtip}/input"
dirout="./my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}-error-none"
[ ! -d ${dirout} ] && mkdir -p ${dirout}

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=7; aa<=7; aa=aa+1))
  do
    for ((bb=1; bb<=1; bb=bb+1))
    do
        cc=$((aa * bb))
        if [ ! -f "${dirout}/mesh_${aa}_${bb}_${rr}-none.xmp" ]; then
          echo mesh_${aa}_${bb}_${rr}-none.txt
          ./waf --run "obss-pd-test --macType=adhoc --gridSize=${aa} --apNum=${cc} --apStep=30 --aptx=true --app=udp --obssAlgo=none --obssLevel=-75 --route=aodv -rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=180 --flowout=${dirout}/mesh_${aa}_${bb}_${rr}-none.xmp --datarate=1e6" &> "${dirout}/mesh_${aa}_${bb}_${rr}-none.txt"
        fi
    done
  done
done



simtip='4_error'
route='aodv'
app='udp'
ratecontrol='const'

rrstart=$1

dirin="./my-simulations/${simtip}/input"
dirout="./my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}-error-mesh"
[ ! -d ${dirout} ] && mkdir -p ${dirout}

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=7; aa<=7; aa=aa+1))
  do
    for ((bb=1; bb<=1; bb=bb+1))
    do
        cc=$((aa * bb))
        if [ ! -f "${dirout}/mesh_${aa}_${bb}_${rr}-mesh.xmp" ]; then
          echo mesh_${aa}_${bb}_${rr}-mesh.txt
          ./waf --run "obss-pd-test --macType=adhoc --gridSize=${aa} --apNum=${cc} --apStep=30 --aptx=true --app=udp --obssAlgo=mesh --obssLevel=-75 --route=aodv -rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=180 --flowout=${dirout}/mesh_${aa}_${bb}_${rr}-mesh.xmp --datarate=1e6" &> "${dirout}/mesh_${aa}_${bb}_${rr}-mesh.txt"
        fi
    done
  done
done