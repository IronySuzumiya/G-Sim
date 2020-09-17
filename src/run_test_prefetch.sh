#!/bin/bash

APP=('pr')
PL=('8' '16')
SPCAP=('16384' '8192')
GRAPHS=('amazon0302' 'citationCiteseer')
PFBATCH=('2' '5' '10')

for app in "${APP[@]}"; do
  make clean && make LARGE_GRAPH=1 ${app}
  for graph in "${GRAPHS[@]}"; do
    for (( plindex = 0; plindex < ${#PL[@]}; plindex++ ))
    do
      pipeline=${PL[$plindex]}
      spcap=${SPCAP[$plindex]}
      for (( pfindex = 0; pfindex < ${#PFBATCH[@]}; pfindex++ ))
      do
        pfbatch=${PFBATCH[$pfindex]}
        inputGraph="$graph"
        fbase="${inputGraph}_${app}_${pipeline}_batch_${pfbatch}"
        IO_OPTIONS="--logfile=${fbase}_log.out"
        READGRAPH_OPTIONS="--graph_path=graphs/${inputGraph}.mtx"
        MEMORY_OPTIONS="--scratchpad_num_lines=$spcap --prefetch_batch_size=$pfbatch"
        SIMULATION_OPTIONS="--num_iter=3 --num_pipelines=$pipeline --app=$app  --vertex_properties=${fbase}_vp.out"
        echo "./g_sim $MEMORY_OPTIONS $READGRAPH_OPTIONS $SIMULATION_OPTIONS > ${fbase}_debug.out 2>&1"
        ./g_sim $MEMORY_OPTIONS $READGRAPH_OPTIONS $SIMULATION_OPTIONS > ${fbase}_debug.out 2>&1
        mv simulator_output.log ${fbase}_log.out
        mv dramsim3.json ${fbase}_dramsim3.json
        mv dramsim3.txt ${fbase}_dramsim3.txt
        mv dramsim3epoch.json ${fbase}_dramsim3epoch.json
      done
    done
  done
done
