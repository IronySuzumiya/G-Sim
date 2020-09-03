graphs=('shl_200' 'gemat11' 'mbeacxc' 'lshp1882' 'lock_700' 'citationCiteseer' 'orani678' 'amazon0302' 'G42')
apps=('bfs' 'cc' 'sssp' 'pr')

for graph in "${graphs[@]}"; do
    if [ ! -d "exp/$graph" ]; then
        mkdir "exp/$graph"
    fi
    for app in "${apps[@]}"; do
        if [ ! -d "exp/$graph/$app" ]; then
            mkdir "exp/$graph/$app"
        fi
    done
done

for graph in "${graphs[@]}"; do
    for app in "${apps[@]}"; do
        mv $graph*$app*.out exp/$graph/$app
    done
done