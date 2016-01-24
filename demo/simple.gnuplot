set title "Simple Plots" font "20"
set key left box
set samples 50
set style data points

set term png

plot [-15:20] sin(x),atan(x),cos(atan(x))