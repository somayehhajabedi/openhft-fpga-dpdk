module ethernet_parser (
    input  logic        clk,
    input  logic        rst,

    // Input stream from MAC
    input  logic [63:0] s_axis_tdata,
    input  logic        s_axis_tvalid,
    output logic        s_axis_tready,
    input  logic        s_axis_tlast,

    // Output stream to dispatcher
    output logic [63:0] m_axis_tdata,
    output logic        m_axis_tvalid,
    input  logic        m_axis_tready,
    output logic        m_axis_tlast,

    // Parsed Ethernet header
    output logic [15:0] ether_type,
    output logic        header_valid
);

    typedef enum logic [1:0] {
        IDLE,
        READ_HEADER_2,
        FORWARD_PAYLOAD
    } state_t;

    state_t state;

    logic [47:0] dst_mac_reg;
    logic [47:0] src_mac_reg;

    always_ff @(posedge clk) begin
        if (rst) begin
            state         <= IDLE;
            dst_mac_reg   <= 48'd0;
            src_mac_reg   <= 48'd0;
            ether_type    <= 16'd0;
            header_valid  <= 1'b0;
            m_axis_tdata  <= 64'd0;
            m_axis_tvalid <= 1'b0;
            m_axis_tlast  <= 1'b0;
        end else begin
            header_valid  <= 1'b0;
            m_axis_tvalid <= 1'b0;
            m_axis_tlast  <= 1'b0;

            if (s_axis_tvalid && s_axis_tready) begin
                case (state)

                    IDLE: begin
                        // First 8 bytes:
                        // dst_mac = bytes 0..5
                        // src_mac first 2 bytes = bytes 6..7
                        dst_mac_reg      <= s_axis_tdata[63:16];
                        src_mac_reg[47:32] <= s_axis_tdata[15:0];

                        state <= READ_HEADER_2;
                    end

                    READ_HEADER_2: begin
                        // Second 8 bytes:
                        // src_mac remaining 4 bytes = bytes 8..11
                        // ether_type = bytes 12..13
                        // payload starts at bytes 14..15
                        src_mac_reg[31:0] <= s_axis_tdata[63:32];
                        ether_type        <= s_axis_tdata[31:16];
                        header_valid      <= 1'b1;

                        // For now, forward the whole second beat.
                        // Later we will align payload properly.
                        m_axis_tdata  <= s_axis_tdata;
                        m_axis_tvalid <= 1'b1;
                        m_axis_tlast  <= s_axis_tlast;

                        state <= FORWARD_PAYLOAD;
                    end

                    FORWARD_PAYLOAD: begin
                        m_axis_tdata  <= s_axis_tdata;
                        m_axis_tvalid <= 1'b1;
                        m_axis_tlast  <= s_axis_tlast;

                        if (s_axis_tlast) begin
                            state <= IDLE;
                        end
                    end

                endcase
            end
        end
    end

    assign s_axis_tready = m_axis_tready;

endmodule
