cd src
make
gtime -f "%e real" ./macogol $1 inputs/1k.pbm outputs/1k_$1_v.pbm
gtime -f "%e real" ./gol $1 inputs/1k.pbm outputs/1k_$1.pbm
./image_compare.sh outputs/1k_$1.pbm outputs/1k_$1_v.pbm
cd ..