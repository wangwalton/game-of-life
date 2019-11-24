cd src
make
/usr/bin/time -f "%e real" ./gol 10000 inputs/1k.pbm outputs/1k.pbm
diff outputs/1k.pbm outputs/1k_verify_out.pbm
cd ..
