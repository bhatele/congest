set xlabel "Message Size (Bytes)"
set ylabel "Latency (us)"
set title "Effect of distance on latencies (Torus - 8 x 8 x 16)"
set logscale x 2
set logscale y 10
set key top left
set size 0.6,0.6
set xtics ("4" 4, "16" 16, "64" 64, "256" 256, "1K" 1024, "4K" 4096, "16K" 16384, "64K" 65536, "256K" 262144, "1M" 1048576)

set terminal postscript eps enhanced color
set output "wicon2_4096.eps"

plot "xt4_hops_4096_8.dat" using 1:($3*1e6) title "8 hops" with linespoints lw 3, \
"xt4_hops_4096_7.dat" using 1:($3*1e6) title "7 hops" with linespoints lw 3, \
"xt4_hops_4096_6.dat" using 1:($3*1e6) title "6 hops" with linespoints lw 3, \
"xt4_hops_4096_5.dat" using 1:($3*1e6) title "5 hops" with linespoints lw 3, \
"xt4_hops_4096_4.dat" using 1:($3*1e6) title "4 hops" with linespoints lw 3, \
"xt4_hops_4096_3.dat" using 1:($3*1e6) title "3 hops" with linespoints lw 3, \
"xt4_hops_4096_2.dat" using 1:($3*1e6) title "2 hops" with linespoints lw 3, \
"xt4_hops_4096_1.dat" using 1:($3*1e6) title "1 hop" with linespoints lw 3
