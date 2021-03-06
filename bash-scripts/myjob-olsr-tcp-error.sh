#!/bin/bash

startTime=10

simtip='4_error'
route='olsr'
app='tcp'
ratecontrol='arf'

rrstart=$1

dirin="my-simulations/${simtip}/input"
dirout="my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}-${rrstart}-error"
[ ! -d ${dirout} ] && mkdir -p ${dirout}

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=1; aa<=9; aa=aa+1))
  do
    for ((bb=1; bb<=aa; bb=bb+1))
    do
        cc=$((aa * bb))
        [ ! ${cc} -gt 40 ] && continue
        if [ ! -f "${dirout}/mesh_${aa}_${bb}_${rr}.xmp" ]; then
          echo mesh_${aa}_${bb}_${rr}.txt
          ./waf --run "mesh-loc-0-1 --app=${app} --startTime=${startTime} --gridSize=${aa} --apNum=${cc} --apStep=60 --aptx=true --gateways=1 --rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=180 --flowout=${dirout}/mesh_${aa}_${bb}_${rr}.xmp --datarate=2e6" &> "${dirout}/mesh_${aa}_${bb}_${rr}.txt"
        fi
    done
  done
done


simtip='4_error'
route='olsr'
app='tcp'
ratecontrol='arf'

rrstart=$1

dirin="my-simulations/${simtip}/input"
dirout="my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}-${rrstart}"
[ ! -d ${dirout} ] && mkdir -p ${dirout}

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=1; aa<=9; aa=aa+1))
  do
    for ((bb=1; bb<=aa; bb=bb+1))
    do
        cc=$((aa * bb))
        [ ! ${cc} -gt 40 ] && continue
        if [ ! -f "${dirout}/mesh_${aa}_${bb}_${rr}.xmp" ]; then
          echo mesh_${aa}_${bb}_${rr}.txt
          ./waf --run "mesh-loc-0 --app=${app} --startTime=${startTime} --gridSize=${aa} --apNum=${cc} --apStep=60 --aptx=true --gateways=1 --rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=180 --flowout=${dirout}/mesh_${aa}_${bb}_${rr}.xmp --datarate=2e6" &> "${dirout}/mesh_${aa}_${bb}_${rr}.txt"
        fi
    done
  done
done


simtip='4_error'
route='olsr'
app='tcp'
ratecontrol='aarf'

rrstart=$1

dirin="my-simulations/${simtip}/input"
dirout="my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}-${rrstart}-error"
[ ! -d ${dirout} ] && mkdir -p ${dirout}

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=1; aa<=9; aa=aa+1))
  do
    for ((bb=1; bb<=aa; bb=bb+1))
    do
        cc=$((aa * bb))
        [ ! ${cc} -gt 40 ] && continue
        if [ ! -f "${dirout}/mesh_${aa}_${bb}_${rr}.xmp" ]; then
          echo mesh_${aa}_${bb}_${rr}.txt
          ./waf --run "mesh-loc-0-1 --app=${app} --startTime=${startTime} --gridSize=${aa} --apNum=${cc} --apStep=60 --aptx=true --gateways=1 --rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=180 --flowout=${dirout}/mesh_${aa}_${bb}_${rr}.xmp --datarate=2e6" &> "${dirout}/mesh_${aa}_${bb}_${rr}.txt"
        fi
    done
  done
done


simtip='4_error'
route='olsr'
app='tcp'
ratecontrol='aarf'

rrstart=$1

dirin="my-simulations/${simtip}/input"
dirout="my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}-${rrstart}"
[ ! -d ${dirout} ] && mkdir -p ${dirout}

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=1; aa<=9; aa=aa+1))
  do
    for ((bb=1; bb<=aa; bb=bb+1))
    do
        cc=$((aa * bb))
        [ ! ${cc} -gt 40 ] && continue
        if [ ! -f "${dirout}/mesh_${aa}_${bb}_${rr}.xmp" ]; then
          echo mesh_${aa}_${bb}_${rr}.txt
          ./waf --run "mesh-loc-0 --app=${app} --startTime=${startTime} --gridSize=${aa} --apNum=${cc} --apStep=60 --aptx=true --gateways=1 --rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=180 --flowout=${dirout}/mesh_${aa}_${bb}_${rr}.xmp --datarate=2e6" &> "${dirout}/mesh_${aa}_${bb}_${rr}.txt"
        fi
    done
  done
done


simtip='4_error'
route='olsr'
app='tcp'
ratecontrol='ideal'

rrstart=$1

dirin="my-simulations/${simtip}/input"
dirout="my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}-${rrstart}-error"
[ ! -d ${dirout} ] && mkdir -p ${dirout}

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=1; aa<=9; aa=aa+1))
  do
    for ((bb=1; bb<=aa; bb=bb+1))
    do
        cc=$((aa * bb))
        [ ! ${cc} -gt 40 ] && continue
        if [ ! -f "${dirout}/mesh_${aa}_${bb}_${rr}.xmp" ]; then
          echo mesh_${aa}_${bb}_${rr}.txt
          ./waf --run "mesh-loc-0-1 --app=${app} --startTime=${startTime} --gridSize=${aa} --apNum=${cc} --apStep=60 --aptx=true --gateways=1 --rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=180 --flowout=${dirout}/mesh_${aa}_${bb}_${rr}.xmp --datarate=2e6" &> "${dirout}/mesh_${aa}_${bb}_${rr}.txt"
        fi
    done
  done
done


simtip='4_error'
route='olsr'
app='tcp'
ratecontrol='ideal'

rrstart=$1

dirin="my-simulations/${simtip}/input"
dirout="my-simulations/${simtip}/output/${route}-${app}-${ratecontrol}-${rrstart}"
[ ! -d ${dirout} ] && mkdir -p ${dirout}

for ((rr=${rrstart}; rr<=${rrstart}; rr=rr+1))
do
  for ((aa=1; aa<=9; aa=aa+1))
  do
    for ((bb=1; bb<=aa; bb=bb+1))
    do
        cc=$((aa * bb))
        [ ! ${cc} -gt 40 ] && continue
        if [ ! -f "${dirout}/mesh_${aa}_${bb}_${rr}.xmp" ]; then
          echo mesh_${aa}_${bb}_${rr}.txt
          ./waf --run "mesh-loc-0 --app=${app} --startTime=${startTime} --gridSize=${aa} --apNum=${cc} --apStep=60 --aptx=true --gateways=1 --rateControl=${ratecontrol} --rndSeed=${rr} --totalTime=180 --flowout=${dirout}/mesh_${aa}_${bb}_${rr}.xmp --datarate=2e6" &> "${dirout}/mesh_${aa}_${bb}_${rr}.txt"
        fi
    done
  done
done
