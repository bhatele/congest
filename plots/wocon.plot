set xlabel "Message Size (Bytes)"
set ylabel "Latency (us)"
set y2label "% Difference"
set title "No Contention Message Latencies (Torus - 8 x 8 x 16)"
set logscale x 2
set logscale y 10
set pointsize 0.8 
set key top center
set size 0.6,0.6
set xtics ("4" 4, "16" 16, "64" 64, "256" 256, "1K" 1024, "4K" 4096, "16K" 16384, "64K" 65536, "256K" 262144, "1M" 1048576)
set y2range [0:30]
set y2tics ("30" 30, "25" 25, "20" 20, "15" 15, "10" 10, "5" 5, "0" 0)

set style line 1 lt rgb "red" lw 3

set terminal postscript eps enhanced color "NimbusSanL-Regu" fontfile "/opt/local/share/texmf-dist/fonts/type1/urw/helvetic/uhvr8a.pfb" 14
#set terminal postscript eps enhanced color "NimbusSanL-Regu" fontfile "/usr/share/texmf-texlive/fonts/type1/urw/helvetic/uhvr8a.pfb" 14
set output "latency_CO_1024.eps"

plot "bgp_latency_1024.dat" using 1:($2*1e6) title "Message Latency" axes x1y1 pt 6 lt 3, \
"bgp_latMAM_1024.dat" using 1:5 title "% difference" axes x1y2 with linespoints ls 1
