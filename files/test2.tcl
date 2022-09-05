power VPWR 
ground VGND
h VPWR
l VGND
vsupply VPWR 1.8

powlogfile /dev/null
powtrace *
powstep
powhist init 0 1
every 10 {powhist capture}
s 10000
powhist print

