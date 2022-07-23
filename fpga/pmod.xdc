set_property PACKAGE_PIN B10 [get_ports pmod[0]] ;# PMOD pin 2 - som240_1_b20  
set_property PACKAGE_PIN E12 [get_ports pmod[1]] ;# PMOD pin 4 - som240_1_b21
set_property PACKAGE_PIN D11 [get_ports pmod[2]] ;# PMOD pin 6 - som240_1_b22
set_property PACKAGE_PIN B11 [get_ports pmod[3]] ;# PMOD pin 8 - som240_1_c22
set_property PACKAGE_PIN H12 [get_ports pmod[4]] ;# PMOD pin 1 - som240_1_a17
set_property PACKAGE_PIN E10 [get_ports pmod[5]] ;# PMOD pin 3 - som240_1_d20
set_property PACKAGE_PIN D10 [get_ports pmod[6]] ;# PMOD pin 5 - som240_1_d21
set_property PACKAGE_PIN C11 [get_ports pmod[7]] ;# PMOD pin 7 - som240_1_d22

set_property IOSTANDARD LVCMOS33 [get_ports pmod*];
set_property SLEW SLOW [get_ports pmod*];
set_property DRIVE 4 [get_ports pmod*];
