cd src
make
gtime -f "%e real" ./gol 500 inputs/1k.pbm outputs/1k_500.pbm
diff outputs/1k_500.pbm outputs/1k_500_verified.pbm
cd ..
