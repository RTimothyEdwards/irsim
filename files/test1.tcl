power vdd1
power vdd2 
h vdd1
h vdd2
ground gnd
l gnd
l in1
l in2
every 10 {toggle in1; toggle in2}
vsupply vdd1 1
vsupply vdd2 2

powlogfile /dev/null
powtrace *
powstep
s 30
