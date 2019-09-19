#!/bin/bash

for ((rrstart=2; rrstart<=20; rrstart=rrstart+1))
do
    echo ${rrstart}
    if [ $1 -eq 1 ]
    then
        ./waf --run "obss-pd-0917 --macType=adhoc --rateControl=const --datarate=35e6 --obssAlgo=none --obssLevel=-62 --app=udp-new --route=static --totalTime=400 --rndSeed=${rrstart} --lengthStep=25 --widthStep=200 --heMcs=HeMcs4" &> $1-${rrstart}.txt
    fi

    if [ $1 -eq 2 ]
    then
        ./waf --run "obss-pd-0917 --macType=adhoc --rateControl=const --datarate=35e6 --obssAlgo=mesh --obssLevel=-62 --app=udp-new --route=static --totalTime=400 --rndSeed=${rrstart} --lengthStep=25 --widthStep=200 --heMcs=HeMcs4" &> $1-${rrstart}.txt
    fi

    if [ $1 -eq 3 ]
    then
        ./waf --run "obss-pd-0917 --macType=adhoc --rateControl=const --datarate=35e6 --obssAlgo=none --obssLevel=-62 --app=udp-new -route=static --totalTime=400 --rndSeed=${rrstart} --lengthStep=25 --widthStep=70 --heMcs=HeMcs4" &> $1-${rrstart}.txt
    fi

    if [ $1 -eq 4 ]
    then
        ./waf --run "obss-pd-0917 --macType=adhoc --rateControl=const --datarate=35e6 --obssAlgo=mesh --obssLevel=-62 --app=udp-new --route=static --totalTime=400 --rndSeed=${rrstart} --lengthStep=25 --widthStep=70 --heMcs=HeMcs4" &> $1-${rrstart}.txt
    fi

    if [ $1 -eq 5 ]
    then
        ./waf --run "obss-pd-0917 --macType=adhoc --rateControl=const --datarate=35e6 --obssAlgo=mesh --obssLevel=-62 --app=udp-new --route=static --totalTime=400 --rndSeed=${rrstart} --lengthStep=25 --widthStep=70 --heMcs=HeMcs3" &> $1-${rrstart}.txt
    fi

    if [ $1 -eq 6 ]
    then
        ./waf --run "obss-pd-0917 --macType=adhoc --rateControl=const --datarate=35e6 --obssAlgo=mesh --obssLevel=-62 --app=udp-new --route=static --totalTime=400 --rndSeed=${rrstart} --lengthStep=25 --widthStep=70 --heMcs=HeMcs2" &> $1-${rrstart}.txt
    fi

    if [ $1 -eq 7 ]
    then
        ./waf --run "obss-pd-0917 --macType=adhoc --rateControl=const --datarate=35e6 --obssAlgo=mesh --obssLevel=-62 --app=udp-new --route=static --totalTime=400 --rndSeed=${rrstart} --lengthStep=25 --widthStep=70 --heMcs=HeMcs1" &> $1-${rrstart}.txt
    fi
done
