#!/bin/bash
echo "Hello World"
# for i in {1..10}
# do
#   echo "${i}回目"
#   ./waf build
#    ./waf --run shutoTestMobility
#     ./waf --run "Lsgo-SimulationScenario --buildings=0  --protocol=5 --lossModel=4 --scenario=3"
# done

Seed=10000
Finish_Seed=10010
while true
do
  echo $Seed
  Seed=`echo "$Seed+1" | bc`
  if [ $Seed -eq $Finish_Seed ]; then
    exit 0
  fi
  ./waf build
  ./waf --run "Lsgo-SimulationScenario --buildings=0  --protocol=6 --lossModel=4 --scenario=3 --nodes=300"
done