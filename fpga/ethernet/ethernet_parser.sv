module ethernet_parser (
    input  logic        clk,
    input  logic        rst,

    // AXI Stream Interfaces
    axis_stream_if.slave  s_axis,
    axis_stream_if.master m_axis,

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
            m_axis.tdata  <= 64'd0;
            m_axis.tvalid <= 1'b0;
            m_axis.tlast  <= 1'b0;
        end else begin
            header_valid  <= 1'b0;
            m_axis.tvalid <= 1'b0;
            m_axis.tlast  <= 1'b0;

            if (s_axis.tvalid && s_axis.tready) begin
                case (state)

                    IDLE: begin
                        // First 8 bytes:
                        // dst_mac = bytes 0..5
                        // src_mac first 2 bytes = bytes 6..7
                        dst_mac_reg      <= s_axis.tdata[63:16];
                        src_mac_reg[47:32] <= s_axis.tdata[15:0];

                        state <= READ_HEADER_2;
                    end

                    READ_HEADER_2: begin
                        // Second 8 bytes:
                        // src_mac remaining 4 bytes = bytes 8..11
                        // ether_type = bytes 12..13
                        // payload starts at bytes 14..15
                        src_mac_reg[31:0] <= s_axis.tdata[63:32];
                        ether_type        <= s_axis.tdata[31:16];
                        header_valid      <= 1'b1;

                        // For now, forward the whole second beat.
                        // Later we will align payload properly.
                        m_axis.tdata  <= s_axis.tdata;
                        m_axis.tvalid <= 1'b1;
                        m_axis.tlast  <= s_axis.tlast;

                        state <= FORWARD_PAYLOAD;
                    end

                    FORWARD_PAYLOAD: begin
                        m_axis.tdata  <= s_axis.tdata;
                        m_axis.tvalid <= 1'b1;
                        m_axis.tlast  <= s_axis.tlast;

                        if (s_axis.tlast) begin
                            state <= IDLE;
                        end
                    end

		    default: begin
   			 state <= IDLE;
			end

                endcase
            end
        end
    end

    assign s_axis.tready = m_axis.tready;

endmodule
