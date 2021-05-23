#!/bin/bash
#simulation start seed
Seed=10000
#simulation finish seed
Finish_Seed=10030
while true
do
  echo $Seed
  Seed=`echo "$Seed+1" | bc`
  if [ $Seed -eq $Finish_Seed ]; then
    exit 0
  fi
  ./waf build
  ./waf --run "Lsgo-SimulationScenario --buildings=0  --protocol=6 --lossModel=4 --scenario=3 --nodes=300 --seed=$Seed"
done