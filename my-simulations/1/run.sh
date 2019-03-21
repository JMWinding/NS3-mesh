#!/bin/bash

dirout="./my-simulations/1/"
anim=true
printRoutes=true
for ((rndSeed=10; rndSeed<=10; rndSeed=rndSeed+1))
do
  for ((apStep=150; apStep<=150; apStep=apStep+10))
  do
    gridSize=$((500/${apStep}+1))
    echo ${gridSize}
    clNum=$((100/${gridSize}/${gridSize}+1))
    echo ${clNum}
    ./waf --command-template="%s --rndSeed=${rndSeed} --gridSize=${gridSize} --clNum=${clNum} --apStep=${apStep} --totalTime=100 --anim=${anim} --printRoutes=${printRouts} --datarate=500Kbps --dirout=${dirout}" --run=mesh-aodv-1 &> ${dirout}aodv.result.${apStep}.${rndSeed}.txt
  done
done
