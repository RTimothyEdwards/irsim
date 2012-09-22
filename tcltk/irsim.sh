#!/bin/sh
#
# For installation, put this file (irsim.sh) in a standard executable path.
# Put startup script "irsim.tcl" and shared library "tclirsim.so"
# in ${CAD_ROOT}/irsim/tcl/, with a symbolic link from file
# ".wishrc" to "irsim.tcl".
#
# This script starts irsim under the Tcl interpreter,
# reading commands from a special .wishrc script which
# launches irsim and retains the Tcl interactive interpreter.

# Parse for the argument "-c[onsole]".  If it exists, run irsim
# with the TkCon console.  Strip this argument from the argument list.

TKCON=true
IRSIM_WISH=/usr/bin/wish
export IRSIM_WISH

# Hacks for Cygwin
if [ ${TERM:=""} = "cygwin" ]; then
   export PATH="$PATH:/usr/lib"
   export DISPLAY=${DISPLAY:=":0"}
fi

for i in $@ ; do
   case $i in
      -noc*) TKCON=;;
   esac
done

if [ $TKCON ]; then

   exec /usr/local/lib/irsim/tcl/tkcon.tcl \
	-eval "source /usr/local/lib/irsim/tcl/console.tcl" \
	-slave "package require Tk; set argc $#; set argv [list $*]; \
	source /usr/local/lib/irsim/tcl/irsim.tcl"

#  exec /usr/local/lib/irsim/tcl/tkcon.tcl -exec "" -eval \
#	"set argc $#; set argv [list $*]; source /usr/local/lib/irsim/tcl/irsim.tcl"

else

#
# Run the stand-in for wish (irsimexec), which acts exactly like "wish"
# except that it replaces ~/.wishrc with irsim.tcl.  This executable is
# *only* needed when running without the console; the console itself is
# capable of sourcing the startup script.
#

   exec /usr/local/lib/irsim/tcl/irsimexec -- $@

fi
