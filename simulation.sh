#!/bin/bash
#simulation start seed
echo -n INPUT_PROTOCOL_ LSGO = 0 SIGO = 1 SIGO_RECOVER = 2 JBR = 3
read protocol
echo $protocol
echo -n NODE_NUM
read node_num
echo -n $node_num
# echo -n START_SEED
# read input_s_seed
# echo $input_s_seed
Seed=10000
#simulation finish seed
# Seed=$input_s_seed
Finish_Seed=10100
while true
do
  echo 'simulation run seed'
  echo $Seed
  Seed=`echo "$Seed+1" | bc`
  if [ $Seed -eq $Finish_Seed ]; then
    exit 0
  fi
    if [ $protocol -eq 1 ]; then #1 = sigo
      ./waf build
      ./waf --run "Lsgo-SimulationScenario --buildings=1  --protocol=8 --lossModel=4 --scenario=3 --nodes=$node_num --seed=$Seed"
    elif [ $protocol -eq 2 ]; then #2 = sigo recovery
      ./waf build
      ./waf --run "Lsgo-SimulationScenario --buildings=1  --protocol=9 --lossModel=4 --scenario=3 --nodes=$node_num --seed=$Seed"
    elif [ $protocol -eq 3 ]; then #3 = jbr
      ./waf build
      ./waf --run "Lsgo-SimulationScenario --buildings=1  --protocol=10 --lossModel=4 --scenario=3 --nodes=$node_num --seed=$Seed"
    else #0 = lsgo
      ./waf build
      ./waf --run "Lsgo-SimulationScenario --buildings=1  --protocol=6 --lossModel=4 --scenario=3 --nodes=$node_num --seed=$Seed"
    fi
done