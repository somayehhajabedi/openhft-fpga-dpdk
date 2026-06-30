interface axis_stream_if #(
    parameter DATA_WIDTH = 64
);

    logic [DATA_WIDTH-1:0] tdata;
    logic                  tvalid;
    logic                  tready;
    logic                  tlast;

endinterface
