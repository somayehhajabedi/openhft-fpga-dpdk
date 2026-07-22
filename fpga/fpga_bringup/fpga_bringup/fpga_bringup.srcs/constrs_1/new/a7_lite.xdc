set_property PACKAGE_PIN J19 [get_ports clk]
set_property IOSTANDARD LVCMOS33 [get_ports clk]
create_clock -period 20.000 -name sys_clk [get_ports clk]

set_property PACKAGE_PIN M18 [get_ports led]
set_property IOSTANDARD LVCMOS33 [get_ports led]