set terminal pngcairo enhanced font "Times,15" fontscale 1.0 size 600, 400 
set output "output/Load_latency_per_10k.jpg" 
set key top outside horizontal center 
# set title "Gunplot test" font "arial,40" 
set xrange [0:120]
set yrange [0:11]
# set xla "x" font "arial,30"
set xlabel "操作次数(百万)" offset 0,0.5 
set ylabel "响应时间(us)"
set xtics axis 0,20,100 nomirror
set ytics axis 0,2,10 nomirror
set zeroaxis
# set key font ", 20"
# set lmargin 10
# set bmargin 4
unset x2tics
unset y2tics
# unset border
# set key
plot "./data/alex_load_latency.dat" using ($1/1e6):($2/1000) with l  title "ALex", \
"./data/fastfair_load_latency.dat" using ($1/1e6):($2/1000) with l  title "Fast-Fair", \
"./data/pgm_load_latency.dat" using ($1/1e6):($2/1000) with dot  title "D-PGM", \
"./data/xindex_load_latency.dat" using ($1/1e6):($2/1000) with l  title "xindex", \
"./data/learngroup_load_latency.dat" using ($1/1e6):($2/1000) with l  title "learngroup", \
"./data/learngroup_expandall_latency.dat" using ($1/1e6):($2/1000) with l  title "learngroup-expandall"