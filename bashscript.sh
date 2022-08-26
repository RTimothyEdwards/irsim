#!/usr/bin/bash
make clean
./configure
sudo make
sudo make install
irsim files/sky130A_1v80_27.prm files/digital_pll.sim -c files/digital_pll.tcl
