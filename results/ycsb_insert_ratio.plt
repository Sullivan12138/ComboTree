# set terminal pngcairo  transparent enhanced font "arial,10" fontscale 1.0 size 600, 400 
set terminal pngcairo  font "arial,15" fontscale 1.0 size 600, 400 
set boxwidth 0.9 absolute
# set style fill   solid 1.00 border lt -1
set key right top vertical Right noreverse noenhanced autotitle nobox
set style histogram clustered gap 1 title textcolor lt -1
set datafile missing '-'
set style data histograms
set style fill pattern 3 border -1
# set style fill solid 1.00 border -1
set xtics border in scale 0,0 nomirror  autojustify
set xtics  norangelimit 

set output 'output/dram_ycsb_insert_ratio.png'
# set title "Load 400 million, operation 10 million, thread 1" 
set yrange [0:3000]
set ylabel "IOPS(KIOPS)" 
set xlabel "插入操作比例"
set title "dram上不同数据结构性能测试"
# set key top outside horizontal right font ",10"
plot 'data/dram_insert_ratio_iops.dat' using 2:xtic(1) ti col, '' u 3 ti col, \
'' u 4 ti col,'' u 5 ti col,'' u 6 ti col, '' u 7 ti col
set output