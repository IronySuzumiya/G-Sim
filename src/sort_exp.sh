graphs=('shl_200' 'gemat11' 'mbeacxc' 'lshp1882' 'lock_700' 'citationCiteseer' 'orani678' 'amazon0302' 'G42')
apps=('bfs' 'cc' 'sssp' 'pr')

if [ ! -d "exp" ]; then
    mkdir "exp"
fi

for app in "${apps[@]}"; do
    if [ ! -d "exp/$app" ]; then
        mkdir "exp/$app"
    fi
    for graph in "${graphs[@]}"; do
        if [ ! -d "exp/$app/$graph" ]; then
            mkdir "exp/$app/$graph"
        fi
    done
done

for app in "${apps[@]}"; do
    for graph in "${graphs[@]}"; do
        mv $graph*$app*.out exp/$app/$graph
        mv $graph*$app*.zip exp/$app/$graph
        mv $graph*$app*.json exp/$app/$graph
        mv $graph*$app*.txt exp/$app/$graph
    done
done