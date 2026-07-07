`timescale 1ns/1ps

module ethernet_parser_tb;

    logic clk;
    logic rst;

    // Clock generation: 100 MHz
    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    // Reset
    initial begin
        rst = 1'b1;
        #20;
        rst = 1'b0;
    end

    // DUT

    // Stimulus

    // Monitor

endmodule
