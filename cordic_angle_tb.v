/*
* Copyright (c) 2025. All rights reserved.
*/

`timescale 1ns / 1ps

module cordic_angle_tb();
    reg i_clk;
    reg i_rst_n;
    reg [15:0] i_y;
    reg [15:0] i_z;
    reg i_start;
    wire o_done;
    wire [15:0] o_angle;

    cordic_angle #(
        .WIDTH(16)
    ) uut (
        .i_clk  (i_clk  ),
        .i_rst_n(i_rst_n),
        .i_y    (i_y    ),
        .i_z    (i_z    ),
        .i_start(i_start),
        .o_done (o_done ),
        .o_angle(o_angle)
    );

    initial begin
        i_clk = 0;
        forever #(10 / 2) i_clk = ~i_clk;
    end

    initial begin
        $dumpfile("dump.vcd");
        $dumpvars;
        i_rst_n = 0;
        i_y = 0;
        i_z = 0;
        i_start = 0;

        #100;
        i_rst_n = 1;
        #20;

        $display("Starting Test Case 1: Y=256, Z=512");

        i_y = 16'd256;
        i_z = 16'd512;

        @(posedge i_clk);
        i_start = 1;
        @(posedge i_clk);
        i_start = 0;

        wait(o_done == 1);
        @(posedge i_clk);
        $display("Test Finished.");
        $display("Inputs: Y=%d, Z=%d", i_y, i_z);
        $display("Output Angle: %d (Expected: ~1897)", $signed(o_angle));
        if ($signed(o_angle) >= 1890 && $signed(o_angle) <= 1910)
            $display("Result: PASS");
        else
            $display("Result: FAIL");
        $finish;
    end
endmodule
