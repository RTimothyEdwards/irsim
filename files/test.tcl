stepsize 50
h vdd
l gnd

w out c b a
d
l a b c
d
s
h c
s
h b 
s
path out
unitdelay 0.1

set vlist {000 001 010 011 100 101 110 111}
foreach vec $vlist {setvector in $vec ; s}

analyzer a b c out

