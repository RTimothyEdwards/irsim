#---------------------------------------------------------
# vcd.tcl
#---------------------------------------------------------
# Support for reading "VCD" format dumpfiles in IRSIM.
# Adds command "readvcd <dumpfile>", which reads in
# the file and displays the traces in the analyzer.
#---------------------------------------------------------

proc readvcd {dumpfile} {
   set prefix ""
   if { [catch {open $dumpfile r} df] } {
      puts stderr "Could not open VCD dumpfile $dumpfile\n"
      return
   }
   while {[gets $df line] >= 0} {
      if {[regexp {^\$([^ ]+)} $line lmatch dumpvar]} {
         switch $dumpvar {
	    date {
	       gets $df line
	       puts stdout $line
	    }
	    version {
	       gets $df line
	       puts stdout $line
	    }
	    timescale {
	       gets $df line
	       regexp {([0-9]+)[ \t]*([^ ]+)} $line lmatch scale metric
	       switch $metric {
	          fs {set scale [expr 0.001 * $scale]}
	          ns {set scale [expr 1000 * $scale]}
	       }
	    }
	    var {
	       regexp {^\$var[ \t]+[^ ]+[ \t]+([0-9]+)[ \t]+([^ ]+)[ \t]+([^ ]+)} \
			$line lmatch bitlen repchar signame
	       if {$bitlen == 1} {
		  addnode ${prefix}${signame}
	       } else {
	          for {set i 0} {$i < $bitlen} {incr i} {
	             addnode ${prefix}${signame}\[$i\]
		  }
		  incr bitlen -1
		  vector ${prefix}${signame} ${prefix}${signame}\[0:${bitlen}\]
	       }
	       set nodenames($repchar) ${prefix}${signame}
	       ana ${prefix}${signame}
	    }
	    scope {
	       regexp {^\$scope[ \t]+([^ ]+)[ \t]+([^ ]+)} \
			$line lmatch scopetype instname
	       set prefix "${prefix}${instname}/"
	    }
	    upscope {
	       set prefix ""
	    }
         }
      } else {
	 # Known patterns are: 0, 1, x (bit set), b (vector set), # (schedule)
	 set curtime 0
	 while {[gets $df line] >= 0} {
	    set cmd [string index $line 0]
	    switch $cmd {
	       b {
	         regexp {^b([0-9]+)[ \t]+([^ ]+)} $line lmatch bval sname
		 setvector $nodenames($sname) %b${bval}
	       }
	       # {
		 set tval [string range $line 1 end]
		 set tval [expr $tval * $scale]
		 set nexttime $tval
		 set tval [expr $tval - $curtime]
		 if {$tval > 0} s $tval
		 set curtime $nexttime
	       }
	       0 {
		 set sname [string range $line 1 end]
		 l $nodenames($sname)
	       }
	       1 {
		 set sname [string range $line 1 end]
		 h $nodenames($sname)
	       }
	       x {
		 set sname [string range $line 1 end]
		 u $nodenames($sname)
	       }
	    }  
	 }
      }
   }

   close $df
}

#---------------------------------------------------------
# For backward compatibility.  Procedure was originally named "readcver",
# but the format is VCD.
#---------------------------------------------------------

proc readcver {dumpfile} {
   readvcd $dumpfile
}
