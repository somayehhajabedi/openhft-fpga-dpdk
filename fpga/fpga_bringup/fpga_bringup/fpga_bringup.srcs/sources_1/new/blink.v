`timescale 1ns / 1ps
module blink (
    input  wire clk,
    output wire led
);

    reg [25:0] counter = 0;

    always @(posedge clk) begin
        counter <= counter + 1'b1;
    end

    assign led = counter[25];

endmodule


