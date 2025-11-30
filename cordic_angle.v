/*
* Copyright (c) 2025. All rights reserved.
*/

module cordic_angle #(
    parameter WIDTH = 16
)(
    input  wire                      i_clk,
    input  wire                      i_rst_n,
    input  wire [WIDTH - 1:0]        i_y,
    input  wire [WIDTH - 1:0]        i_z,
    input  wire                      i_start,
    output wire                      o_done,
    output wire [WIDTH - 1:0]        o_angle
);
    // --- Angle Look-Up Table ---
    wire [WIDTH - 1:0] c_A [12:0];

    assign c_A[0] = 3217;
    assign c_A[1] = 1899;
    assign c_A[2] = 1003;
    assign c_A[3] = 509;
    assign c_A[4] = 256;
    assign c_A[5] = 128;
    assign c_A[6] = 64;
    assign c_A[7] = 32;
    assign c_A[8] = 16;
    assign c_A[9] = 8;
    assign c_A[10] = 4;
    assign c_A[11] = 2;
    assign c_A[12] = 1;

    // --- Cordic Datapath ---
    reg [WIDTH - 1:0] d_U;
    reg [WIDTH - 1:0] d_V;
    reg [WIDTH - 1:0] d_angle;
    reg d_done;
    wire d_sign;

    // --- FSM Implementation ---
    localparam initialState = 1'b0, countState = 1'b1;
    reg currentState, nextState;
    reg [3:0] count;

    always @(posedge i_clk, negedge i_rst_n) begin
        if (i_rst_n == 1'b0) begin
            currentState <= initialState;
            nextState <= initialState;
            count <= 4'd0;
        end else begin
            if (currentState == initialState && nextState == countState)
                count <= 4'd0;
            else if (currentState == countState)
                count <= count + 4'd1;
            currentState <= nextState;
        end
    end

    always @(currentState, i_start, count) begin
        if (currentState == initialState && i_start == 1'b1)
            nextState = countState;
        else if (currentState == countState && count == 4'd12)
            nextState = initialState;
        else
            nextState = currentState;
    end

    // --- Cordic Cycle ---
    assign d_sign = d_V[WIDTH - 1];

    always @(posedge i_clk, negedge i_rst_n) begin
        if (i_rst_n == 1'b0) begin
            d_U <= 0;
            d_V <= 0;
            d_angle <= 0;
        end
        else if (currentState == initialState && nextState == countState) begin
            d_U <= i_z;
            d_V <= i_y;
            d_angle <= 0;
        end else if (currentState == countState) begin
            // sign = +1
            if (d_sign == 1'b0) begin
                d_U <= $signed(d_U) + ($signed(d_V) >>> count);
                d_V <= $signed(d_V) - ($signed(d_U) >>> count);
                d_angle <= $signed(d_angle) + c_A[count];
            end
            // sign = -1
            else begin
                d_U <= $signed(d_U) - ($signed(d_V) >>> count);
                d_V <= $signed(d_V) + ($signed(d_U) >>> count);
                d_angle <= $signed(d_angle) - c_A[count];
            end
        end
    end

    always @(posedge i_clk, negedge i_rst_n) begin
        if (i_rst_n == 1'b0) begin
            d_done <= 1'b0;
        end else begin
            if (currentState == countState && nextState == initialState && count == 4'd12)
                d_done <= 1'b1;
            else if (currentState == initialState && nextState == countState)
                d_done <= 1'b0;
        end
    end

    assign o_done = d_done;
    assign o_angle = d_angle;
endmodule
