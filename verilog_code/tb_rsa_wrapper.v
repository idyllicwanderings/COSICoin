`timescale 1ns / 1ps

`define HUGE_WAIT   300
`define LONG_WAIT   100
`define RESET_TIME   25
`define CLK_PERIOD   10
`define CLK_HALF      5

module tb_rsa_wrapper();
    
  reg           clk         ;
  reg           resetn      ;
  wire          leds        ;

  reg  [16:0]   mem_addr    = 'b0 ;
  reg  [1023:0] mem_din     = 'b0 ;
  wire [1023:0] mem_dout    ;
  reg  [127:0]  mem_we      = 'b0 ;

  reg  [ 11:0] axil_araddr  ;
  wire         axil_arready ;
  reg          axil_arvalid ;
  reg  [ 11:0] axil_awaddr  ;
  wire         axil_awready ;
  reg          axil_awvalid ;
  reg          axil_bready  ;
  wire [  1:0] axil_bresp   ;
  wire         axil_bvalid  ;
  wire [ 31:0] axil_rdata   ;
  reg          axil_rready  ;
  wire [  1:0] axil_rresp   ;
  wire         axil_rvalid  ;
  reg  [ 31:0] axil_wdata   ;
  wire         axil_wready  ;
  reg  [  3:0] axil_wstrb   ;
  reg          axil_wvalid  ;
      
  tb_rsa_project_wrapper dut (
    .clk                 ( clk           ),
    .leds                ( leds          ),
    .resetn              ( resetn        ),
    .s_axi_csrs_araddr   ( axil_araddr   ),
    .s_axi_csrs_arready  ( axil_arready  ),
    .s_axi_csrs_arvalid  ( axil_arvalid  ),
    .s_axi_csrs_awaddr   ( axil_awaddr   ),
    .s_axi_csrs_awready  ( axil_awready  ),
    .s_axi_csrs_awvalid  ( axil_awvalid  ),
    .s_axi_csrs_bready   ( axil_bready   ),
    .s_axi_csrs_bresp    ( axil_bresp    ),
    .s_axi_csrs_bvalid   ( axil_bvalid   ),
    .s_axi_csrs_rdata    ( axil_rdata    ),
    .s_axi_csrs_rready   ( axil_rready   ),
    .s_axi_csrs_rresp    ( axil_rresp    ),
    .s_axi_csrs_rvalid   ( axil_rvalid   ),
    .s_axi_csrs_wdata    ( axil_wdata    ),
    .s_axi_csrs_wready   ( axil_wready   ),
    .s_axi_csrs_wstrb    ( axil_wstrb    ),
    .s_axi_csrs_wvalid   ( axil_wvalid   ),
    .mem_clk             ( clk           ), 
    .mem_addr            ( mem_addr      ),     
    .mem_din             ( mem_din       ), 
    .mem_dout            ( mem_dout      ), 
    .mem_en              ( 1'b1          ), 
    .mem_rst             (~resetn        ), 
    .mem_we              ( mem_we        ));
      
  // Generate Clock
  initial begin
      clk = 0;
      forever #`CLK_HALF clk = ~clk;
  end

  // Initialize signals to zero
  initial begin
    axil_araddr  <= 'b0;
    axil_arvalid <= 'b0;
    axil_awaddr  <= 'b0;
    axil_awvalid <= 'b0;
    axil_bready  <= 'b0;
    axil_rready  <= 'b0;
    axil_wdata   <= 'b0;
    axil_wstrb   <= 'b0;
    axil_wvalid  <= 'b0;
  end

  // Reset the circuit
  initial begin
      resetn = 0;
      #`RESET_TIME
      resetn = 1;
  end

  // Read from specified register
  task reg_read;
    input [11:0] reg_address;
    output [31:0] reg_data;
    begin
      // Channel AR
      axil_araddr  <= reg_address;
      axil_arvalid <= 1'b1;
      wait (axil_arready);
      #`CLK_PERIOD;
      axil_arvalid <= 1'b0;
      // Channel R
      axil_rready  <= 1'b1;
      wait (axil_rvalid);
      reg_data <= axil_rdata;
      #`CLK_PERIOD;
      axil_rready  <= 1'b0;
      $display("reg[%x] <= %x", reg_address, reg_data);
      #`CLK_PERIOD;
      #`RESET_TIME;
    end
  endtask

  // Write to specified register
  task reg_write;
    input [11:0] reg_address;
    input [31:0] reg_data;
    begin
      // Channel AW
      axil_awaddr <= reg_address;
      axil_awvalid <= 1'b1;
      // Channel W
      axil_wdata  <= reg_data;
      axil_wstrb  <= 4'b1111;
      axil_wvalid <= 1'b1;
      // Channel AW
      wait (axil_awready);
      #`CLK_PERIOD;
      axil_awvalid <= 1'b0;
      // Channel W
      wait (axil_wready);
      #`CLK_PERIOD;
      axil_wvalid <= 1'b0;
      // Channel B
      axil_bready <= 1'b1;
      wait (axil_bvalid);
      #`CLK_PERIOD;
      axil_bready <= 1'b0;
      $display("reg[%x] <= %x", reg_address, reg_data);
      #`CLK_PERIOD;
      #`RESET_TIME;
    end
  endtask

  // Read at given address in memory
  task mem_write;
    input [  16:0] address;
    input [1024:0] data;
    begin
      mem_addr <= address;
      mem_din  <= data;
      mem_we   <= {128{1'b1}};
      #`CLK_PERIOD;
      mem_we   <= {128{1'b0}};
      $display("mem[%x] <= %x", address, data);
      #`CLK_PERIOD;
    end
  endtask

  // Write to given address in memory
  task mem_read;
    input [  16:0] address;
    begin
      mem_addr <= address;
      #`CLK_PERIOD;
      $display("mem[%x] => %x", address, mem_dout);
    end
  endtask

  // Byte Addresses of 32-bit registers
  localparam  COMMAND = 0, // r0
              STATUS  = 0;

  // Byte Addresses of 1024-bit distant memory locations
//  localparam  MEM0_ADDR  = 16'h000,
//              MEM1_ADDR  = 16'h080;

              
  localparam  B = 16'd0,
              R = 16'd128;

              
  localparam  RXB = 4,
              RXF = 8, // shows if next or not
              TXR = 12;

  reg [31:0] reg_status;

  initial begin

    #`LONG_WAIT

    reg_write(4, 32'h0);
    reg_write(8, 32'h80000000);
    reg_write(12, 32'h6e6f7071);
    reg_write(16, 32'h6d6e6f70);
    reg_write(20, 32'h6c6d6e6f);
    reg_write(24, 32'h6b6c6d6e);
    reg_write(28, 32'h6a6b6c6d);
    


    reg_write(COMMAND, 32'h00000003);
    #`CLK_PERIOD;
    reg_write(COMMAND, 32'h7);
    reg_write(4, 32'h696a6b6c);
    reg_write(8, 32'h68696a6b);
    reg_write(12, 32'h6768696a);
    reg_write(16, 32'h66676869);
    reg_write(20, 32'h65666768);
    reg_write(24, 32'h64656667);
    reg_write(28, 32'h63646566);
    reg_write(COMMAND, 32'hb);
    reg_write(4, 32'h62636465);
    reg_write(8, 32'h61626364);
    reg_write(12, 32'h000001c0);
    reg_write(16, 32'd0);
    reg_write(20, 32'd0);
    reg_write(24, 32'd0);
    reg_write(28, 32'd0);
    reg_write(COMMAND, 32'h13);
      reg_write(4, 32'd0);
    reg_write(8, 32'd0);
    reg_write(12, 32'd0);
    reg_write(16, 32'd0);
    reg_write(20, 32'd0);
    reg_write(24, 32'd0);
    reg_write(28, 32'h0);
    reg_write(COMMAND, 32'h23);
    reg_write(4, 32'h0);
    reg_write(8, 32'h0);
    reg_write(12, 32'h0);
    reg_write(16, 32'h0);
    reg_write(20, 32'h0);
    reg_write(24, 32'h0);
    reg_write(28, 32'h0);
    reg_write(COMMAND, 32'h43);
    reg_read(STATUS, reg_status);
    while (reg_status[3] == 1'd0)
        reg_read(STATUS, reg_status);
    begin
        #`LONG_WAIT;
    end
    reg_write(COMMAND, 32'h81);
    #`CLK_PERIOD;
    reg_write(COMMAND, 32'h101);
    #`CLK_PERIOD;
    reg_write(COMMAND, 32'h201);
    

//    while (reg_status[0]==1'b0)
//    begin
//      #`LONG_WAIT;
      
//    end
    
    reg_write(COMMAND, 32'h00000000);

    
    
    
    

    $finish;

  end
endmodule