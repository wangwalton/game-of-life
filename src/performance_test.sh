echo "original implementation"
gtime -f "%e real" ./ogol $1 inputs/1k.pbm outputs/1k.pbm

echo "my implementation"
gtime -f "%e real" ./gol $1 inputs/1k.pbm outputs/1k.pbm