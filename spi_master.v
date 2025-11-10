module spi_master(
    // Control signals
    input i_Rst_L,
    input i_Clk,
    input [1:0] i_spi_mode,
    input [15:0] i_clk_scale, // MUST BE >=2
    input [2:0] i_cs_inactive_clks,
    // TX signals
    input [4:0] i_TX_Count,
    input [7:0] i_TX_Byte,
    input i_TX_DV,
    output o_TX_Ready,
    // RX signals
    output [3:0] o_RX_Count,
    output o_RX_DV,
    output [7:0] o_RX_Byte,
    // SPI signals
    output o_SPI_Clk,
    input i_SPI_MISO,
    output o_SPI_MOSI,
    output o_SPI_CS_n
);
//===============Control========================
    wire cpol;
    wire cpha;
    assign cpol = i_spi_mode[1];
    assign cpha = i_spi_mode[0];

    // indicates an active SPI transaction
    wire spi_ss;

//===============Clock generation logic==========
    wire [15:0] spi_period;
    reg [15:0] spi_counter;
    wire spi_clk;
    wire spi_clk_posedge;
    wire spi_clk_negedge;

    assign spi_period = (i_clk_scale == 16'd0) ? 16'd0 : (i_clk_scale >> 1);

    always @(posedge i_Clk, negedge i_Rst_L) begin
        if (i_Rst_L == 1'b0)
            spi_counter <= 16'd0;
        else if (spi_counter == i_clk_scale - 1)
            spi_counter <= 16'd0;
        else
            spi_counter <= spi_counter + 16'd1;
    end

    assign spi_clk = spi_counter >= spi_period;
    assign spi_clk_posedge = (spi_counter == spi_period);
    assign spi_clk_negedge = (spi_counter == 16'd0);

    assign o_SPI_Clk = spi_ss ? (cpol ? ~spi_clk : spi_clk) : cpol;

//===============Byte logic================
    localparam initialState = 2'b00, countState = 2'b01, transferState = 2'b10, cooldownState = 2'b11;
    reg [1:0] currentState;
    reg [1:0] nextState;
    reg [3:0] count;
    reg [3:0] maxCount;
    reg [2:0] spiCount;

    reg [7:0] spi_byte;

    // indicates if a byte has been transferred
    wire spi_byte_done;

    wire spi_toggle_edge;
    wire spi_sample_edge;

    always @(posedge i_Clk, negedge i_Rst_L) begin
        if (i_Rst_L == 1'b0) begin
            currentState <= initialState;
            count <= 4'd0;
            maxCount <= 4'd0;
            spi_byte <= 8'd0;
        end else begin
            if (currentState == initialState && nextState == transferState) begin
                maxCount <= i_TX_Count - 5'd1;
                spi_byte <= i_TX_Byte;
            end else if (currentState == countState && nextState == transferState)
                spi_byte <= i_TX_Byte;
            else if ((currentState == transferState && nextState == countState) || (currentState == cooldownState))
                count <= count + 4'd1;
            else if ((currentState != cooldownState && nextState == cooldownState) || (currentState != initialState && nextState == initialState))
                count <= 4'd0;

            currentState <= nextState;
        end
    end

    always @(currentState, i_TX_Count, i_TX_DV, spi_byte_done, count, maxCount, i_cs_inactive_clks) begin
        nextState = currentState;

        if (currentState == initialState && i_TX_Count > 5'd0 && i_TX_DV)
            nextState = transferState;
        else if (currentState == countState && i_TX_DV)
            nextState = transferState;
        else if (currentState == transferState && spi_byte_done)
            if (count == maxCount)
                nextState = cooldownState;
            else 
                nextState = countState;
        else if (currentState == cooldownState && count == i_cs_inactive_clks)
            nextState = initialState;
    end

    always @(posedge i_Clk, negedge i_Rst_L) begin
        if (i_Rst_L == 1'b0)
            spiCount <= 3'd0;
        else begin
            if (currentState == transferState) begin
                if (spi_sample_edge)
                    if (spiCount == 3'd7)
                        spiCount <= 3'd0;
                    else
                        spiCount <= spiCount + 1'b1;
            end else
                spiCount <= 3'd0;
        end
    end

    assign spi_ss = (currentState != initialState && currentState != cooldownState);
    assign o_TX_Ready = (currentState != transferState && currentState != cooldownState);

//===============Transaction logic===============
    reg [7:0] tx_shreg;
    reg tx_loaded;
    reg [7:0] rx_shreg;

    assign spi_toggle_edge = (spi_clk_posedge && (cpol != cpha)) || (spi_clk_negedge && (cpol == cpha));
    assign spi_sample_edge = (spi_clk_posedge && (cpol == cpha)) || (spi_clk_negedge && (cpol != cpha));
    assign spi_byte_done = (currentState == transferState) && (spiCount == 3'd7) && (spi_sample_edge);

    always @(posedge i_Clk, negedge i_Rst_L) begin
        if (i_Rst_L == 1'b0) begin
            tx_shreg <= 8'd0;
            tx_loaded <= 1'b0;
            rx_shreg <= 8'd0;
        end else begin
            // start of transfer
            if (currentState != transferState && nextState == transferState) begin
                if (!cpha) begin
                    tx_shreg <= i_TX_Byte; // spi_byte isn't set yet
                    tx_loaded <= 1'b1;
                    rx_shreg <= 8'd0;
                end else begin    
                    tx_shreg <= 8'd0;
                    tx_loaded <= 1'b0;
                    rx_shreg <= 8'd0;
                end
            end
            else begin
                if (spi_sample_edge)
                    rx_shreg <= {rx_shreg[6:0], i_SPI_MISO};
                if (spi_toggle_edge) begin
                    if (!tx_loaded) begin
                        tx_shreg <= spi_byte;
                        tx_loaded <= 1'b1;
                    end else
                        tx_shreg <= {tx_shreg[6:0], 1'b0};
                end
            end
        end
    end

    assign o_SPI_MOSI = spi_ss ? tx_shreg[7] : 1'bz;
    assign o_SPI_CS_n = ~spi_ss;

//===============Receive logic=================

    reg [3:0] rx_count;
    reg rx_dv;
    reg [7:0] rx_byte;

    always @(posedge i_Clk, negedge i_Rst_L) begin
        if (i_Rst_L == 1'b0) begin
            rx_count <= 4'd0;
            rx_dv <= 1'b0;
            rx_byte <= 8'd0;
        end
        else begin
            rx_dv <= 1'b0; // pulse for multi-byte rx

            if (spi_byte_done) begin
                rx_count <= count;
                rx_dv <= 1'b1;
                rx_byte <= rx_shreg;
            end
        end
    end

    assign o_RX_Count = rx_count;
    assign o_RX_DV = rx_dv;
    assign o_RX_Byte = rx_byte;
endmodule 