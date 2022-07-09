# IRSIM stimulus for digital_pll
#
# I/O for the digital_pll from the verilog source:
#
#   input        resetb;        // Sense negative reset
#   input        enable;        // Enable PLL
#   input        osc;           // Input oscillator to match
#   input [4:0]  div;           // PLL feedback division ratio
#   input        dco;           // Run in DCO mode
#   input [25:0] ext_trim;      // External trim for DCO mode
#
#   output [1:0] clockp;        // Two 90 degree clock phases

# Set power supplies

l VGND 
h VPWR

# Define signal vectors

vector div div\[4:0\]
vector ext_trim ext_trim\[25:0\]
vector clockp clockp\[1:0\]

# Watch these signals

analyzer
ana osc div clockp resetb enable clockp\[0\] clockp\[1\]

# Initial values

setvector div 0d8
setvector ext_trim 0d9999

h dco
l enable
l resetb
l osc

# To be done:  Figure out why the zero resistor isn't seen on
# the constant high/low outputs.  For now, force the values
h ringosc.iss.const1/HI
l ringosc.iss.const1/LO

# Startup sequence

s 500
h resetb
s 500
h enable
s 500

# Define external clock

every 1000 {toggle osc}

# Continue simulation manually
