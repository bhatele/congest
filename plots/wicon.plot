set xlabel "Message Size (Bytes)"
set ylabel "Latency (us)"
set title "Latency vs. Msg Size: With Contention (4K cores, BG/P)"
set logscale x 2
set logscale y 10
set key top left
set size 0.6,0.6
set xrange [4096:1048576]
set xtics ("4" 4, "16" 16, "64" 64, "256" 256, "1K" 1024, "4K" 4096, "16K" 16384, "64K" 65536, "256K" 262144, "1M" 1048576)

set terminal postscript eps enhanced color
set output "wicon_4096.eps"

set style line 1 lt rgb "red" lw 3 pt 2
set style line 2 lt rgb "blue" lw 3 pt 6

plot "xt4_rnd_4096.dat" using 1:($3*1e6) title "RND: Avg" with linespoints ls 1, \
"xt4_nn_4096.dat" using 1:($3*1e6) title "NN: Avg" with linespoints ls 2
