# Tcl commands to run in the console before IRSIM is initialized
#
puts stdout "Running IRSIM Console Functions"
bind .text <Control-Key-c> {irsim::interrupt}
