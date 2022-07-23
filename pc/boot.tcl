# Xilinx Tcl Reference:
# https://docs.xilinx.com/v/u/2019.2-English/ug835-vivado-tcl-commands

proc boot_jtag { } {
############################
# Switch to JTAG boot mode #
############################
targets -set -filter {name =~ "PSU"}
# update multiboot to ZERO
mwr 0xffca0010 0x0
# change boot mode to JTAG
mwr 0xff5e0200 0x0100
# reset
rst -system
}

proc boot_qspi { } {
############################
# Switch to QSPI boot mode #
############################
targets -set -filter {name =~ "PSU"}
# update multiboot to ZERO
mwr 0xffca0010 0x0
# change boot mode to QSPI
mwr 0xff5e0200 0x2100
# reset
rst -system
#A53 may be held in reset catch, start it with "con"
after 2000
con
}

proc boot_sd { } {
############################
# Switch to SD boot mode #
############################
targets -set -filter {name =~ "PSU"}
# update multiboot to ZERO
mwr 0xffca0010 0x0
# change boot mode to SD
mwr 0xff5e0200 0xE100
# reset
rst -system
#A53 may be held in reset catch, start it with "con"
after 2000
con
}

# start_jtag <bitstream_name> <platform_name> <app_name>
# Example: 
#   start_jtag taylordp taylordp_platform taylordp 
proc start_jtag {bit platform app} {
# check if every file exists before doing anything
set bitfile "$platform/hw/$bit.bit"
set pmufwfile "$platform/export/$platform/sw/$platform/boot/pmufw.elf"
set fsblfile "$platform/export/$platform/sw/$platform/boot/fsbl.elf"
set appfile "$app/Debug/$app.elf"

if { [file exists "$bitfile"] == 0} {
    puts "Cannot find bitstream: $bitfile"
    return 
}
if { [file exists "$pmufwfile"] == 0} {
    puts "Cannot find pmufw: $pmufwfile"
    return
}
if { [file exists "$fsblfile"] == 0} {
    puts "Cannot find fsbl: $fsblfile"
    return
}
if { [file exists "$appfile"] == 0} {
    puts "Cannot find app: $appfile"
    return
}

connect
boot_jtag

after 2000
targets -set -filter {name =~ "PSU"}
fpga "$bitfile"
mwr 0xffca0038 0x1FF

# Download pmufw.elf
for {set i 1} {$i <= 10} {incr i} {
	if {[catch {targets -set -filter {name =~ "MicroBlaze PMU"}} errmsg]} {
		puts "targets failed: $errmsg"
		if {$i == 10} {
			error "Too many errors"
		}
		puts "Retrying #$i"
	} else {
		break
	}
}
after 500
dow "$pmufwfile"
con
after 500

# Select A53 Core 0
targets -set -filter {name =~ "Cortex-A53 #0"}
rst -processor -clear-registers
dow "$fsblfile"
con
after 5000
stop

dow "$appfile"
after 500
con
}

proc load_app {platform app} {
	set appfile "$app/Debug/$app.elf"
	set fsblfile "$platform/export/$platform/sw/$platform/boot/fsbl.elf"
	targets -set -filter {name =~ "Cortex-A53 #0"}
	rst -processor -clear-registers
	dow "$fsblfile"
	con
	after 10000
	stop

	dow "$appfile"
	after 500
	con
}