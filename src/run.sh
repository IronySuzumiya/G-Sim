#!/bin/bash

#APP=('bfs' 'cc' 'sssp' 'pr')
#APP=('cc' 'sssp' 'pr')
APP=('pr')
PL=('1' '8' '16' '32')
SPCAP=('131072', '16384', '8192', '4096')
GRAPHS=('shl_200' 'gemat11' 'mbeacxc' 'lshp1882' 'lock_700' 'orani678' 'G42')
GRAPHS_LARGE=('citationCiteseer' 'amazon0302')
#GRAPHS_LARGE=('soc_Slashdot0902' 'orani678' 'amazon0302' 'G42')

for app in "${APP[@]}"; do
  make clean && make LARGE_GRAPH=1 ${app}
  for graph in "${GRAPHS[@]}"; do
    for index in "$(seq 0 ${#PL[@]})"; do
      pipeline=${PL[$index]}
      spcap=${SPCAP[$index]}
      inputGraph="$graph"
      fbase="${inputGraph}_${app}_${pipeline}"
      IO_OPTIONS="--logfile=${fbase}_log.out"
      READGRAPH_OPTIONS="--graph_path=graphs/${inputGraph}.mtx"
      SIMULATION_OPTIONS="--num_iter=8 --num_pipelines=$pipeline --scratchpad_num_lines=$spcap --app=$app --vertex_properties=${fbase}_vp.out"
      #`rm -f trace/*.csv`
      echo "./g_sim $DRAM_OPTIONS $READGRAPH_OPTIONS $SIMULATION_OPTIONS 2>&1 ${fbase}_run.out"
      ./g_sim $DRAM_OPTIONS $READGRAPH_OPTIONS $SIMULATION_OPTIONS 2>&1 ${fbase}_run.out
      #if [ $? -eq 0 ]; then
      #  `rm -f trace/*_out.csv`
      #  zip -r ${fbase}_trace.zip trace
      #fi
      mv simulator_output.log ${fbase}_log.out
      mv dramsim3.json ${fbase}_dramsim3.json
      mv dramsim3.txt ${fbase}_dramsim3.txt
      #mv dramsim3epoch.json ${fbase}_dramsim3epoch.json
    done
  done
  for graph in "${GRAPHS_LARGE[@]}"; do
    for index in "$(seq 0 ${#PL[@]})"; do
      pipeline=${PL[$index]}
      spcap=${SPCAP[$index]}
      inputGraph="$graph"
      fbase="${inputGraph}_${app}_${pipeline}"
      IO_OPTIONS="--logfile=${fbase}_log.out"
      READGRAPH_OPTIONS="--graph_path=graphs/${inputGraph}.mtx"
      SIMULATION_OPTIONS="--num_iter=3 --num_pipelines=$pipeline --scratchpad_num_lines=$spcap --app=$app --vertex_properties=${fbase}_vp.out"
      #`rm -f trace/*.csv`
      echo "./g_sim $DRAM_OPTIONS $READGRAPH_OPTIONS $SIMULATION_OPTIONS 2>&1 ${fbase}_run.out"
      ./g_sim $DRAM_OPTIONS $READGRAPH_OPTIONS $SIMULATION_OPTIONS 2>&1 ${fbase}_run.out
      #if [ $? -eq 0 ]; then
      #  `rm -f trace/*_out.csv`
      #  zip -r ${fbase}_trace.zip trace
      #fi
      mv simulator_output.log ${fbase}_log.out
      mv dramsim3.json ${fbase}_dramsim3.json
      mv dramsim3.txt ${fbase}_dramsim3.txt
      #mv dramsim3epoch.json ${fbase}_dramsim3epoch.json
    done
  done
done
