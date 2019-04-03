#!/bin/bash

anim=false
printRoutes=false
dirout="./my-simulations/1/"
for ((rndSeed=1; rndSeed<=10; rndSeed=rndSeed+1))
do
  for apStep in 50 66 80 100
  do
    echo ${apStep} ${rndSeed}
    gridSize=$((400/${apStep}))
    clNum=$((200/${gridSize}/${gridSize}))
    ./waf --command-template="%s --rndSeed=${rndSeed} --gridSize=${gridSize} --clNum=${clNum} --apStep=${apStep} --totalTime=60 --anim=${anim} --printRoutes=${printRouts} --datarate=256Kbps --dirout=${dirout}" --run=mesh-aodv-1 &> ${dirout}aodv.result.${apStep}.${rndSeed}.txt
  done
done
