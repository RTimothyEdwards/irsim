#!/usr/bin/tclsh

powlogfile /dev/null
powtrace osc resetb enable clockp\[0\] clockp\[1\]
powstep
vsupply 1.8
powhist init 0 1
every 10 {powhist capture}
s 100
powhist print
