interface axis_stream_if #(
    parameter DATA_WIDTH = 64
);

    logic [DATA_WIDTH-1:0] tdata;
    logic                  tvalid;
    logic                  tready;
    logic                  tlast;

    modport master (
        output tdata,
        output tvalid,
        output tlast,
        input  tready
    );

    modport slave (
        input  tdata,
        input  tvalid,
        input  tlast,
        output tready
    );

endinterface
