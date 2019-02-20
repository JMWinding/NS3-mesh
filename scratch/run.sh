#!/bin/bash

../waf --command-template="%s --RngRun=1 --apNum=3 --clNum=2 --totalTime=20" --run=mesh-aodv-4
