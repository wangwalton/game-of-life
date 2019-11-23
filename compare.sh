./gol 100 inputs/1k.pbm outputs/1k_100_test.pbm
diff outputs/1k_100_test.pbm outputs/1k_100.pbm > diff.out
cat diff.out