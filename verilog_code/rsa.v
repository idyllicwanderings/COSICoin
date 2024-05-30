`timescale 1ns / 1ps

module rsa(
        input  wire          clk,
        input  wire          resetn,
        output wire   [ 3:0] leds,


        // input registers                     // output registers
        input  wire   [31:0] rin0,             output wire   [31:0] rout0,
        input  wire   [31:0] rin1,             output wire   [31:0] rout1,
        input  wire   [31:0] rin2,             output wire   [31:0] rout2,
        input  wire   [31:0] rin3,             output wire   [31:0] rout3,
        input  wire   [31:0] rin4,             output wire   [31:0] rout4,
        input  wire   [31:0] rin5,             output wire   [31:0] rout5,
        input  wire   [31:0] rin6,             output wire   [31:0] rout6,
        input  wire   [31:0] rin7,             output wire   [31:0] rout7,

        // dma signals direct memory access
                
        // dma_rx_adress used to be wire
        input  wire [1023:0] dma_rx_data,      output wire [1023:0] dma_tx_data,
        output reg [  31:0]  dma_rx_address,   output wire [  31:0] dma_tx_address,
        output reg           dma_rx_start,     output reg           dma_tx_start,
        input  wire          dma_done,
        input  wire          dma_idle,
        input  wire          dma_error
    );

    wire [31:0] command;
    assign command        = rin0; // use rin0 as command

    // number of blocks of 512, number of bits can be expanded
    wire num_blocks;
    assign num_blocks = rin0[1];
    
    // wire to check if new data can be read
    wire next_input2 = rin0[2];
    wire next_input3 = rin0[3];
    wire next_input4 = rin0[4];
    wire next_input5 = rin0[5];
    wire next_input6 = rin0[6];
    wire next_output1 = rin0[7];
    wire next_output2 = rin0[8];
    wire next_output3 = rin0[9];


    wire [31:0] status;
    assign rout0 = status; // use rout0 as status

    wire isCmdComp = (command[0] == 1'd1);
    wire isCmdIdle = (command[0] == 1'd0);

    // Define state machine's states
    localparam
        STATE_IDLE          = 4'd14,
        STATE_RIN1          = 4'd1,
        STATE_RIN2          = 4'd2,
        STATE_RIN3          = 4'd3,
        STATE_RIN4          = 2'd1,
        STATE_RIN5          = 2'd2,
        STATE_COMPUTE       = 4'd6,
        STATE_COMPUTE_WAIT  = 4'd7,
        STATE_NEXT          = 4'd8,
        STATE_NEXT_WAIT     = 4'd9,
        STATE_ROUT1         = 4'd10,
        STATE_ROUT2         = 4'd11,
        STATE_DONE          = 4'd12,
        STATE_WAIT          = 4'd13;
    
    // The state machine
    reg [3:0] state = STATE_IDLE; 
    reg [3:0] next_state;
    reg [1:0] state_parallel = 2'd0;
    reg [1:0] next_state_parallel;
    // control signal for compute next
    reg block2_ready = 1'd0;
    // show that parallel state must start
    reg start_parallel = 1'd0;
    // registers for input blocks
    reg [511:0] block1 = 512'd0;
    reg [511:0] block2 = 512'd0;

    // inputs and outputs sha module
    reg          op_reset;
    reg          op_init;
    reg          op_next;
    wire         op_mode;
    reg [511:0]  op_block;
    wire         op_ready;
    wire [255:0] op_result;
    wire         op_result_ready;

    assign op_mode = 1'd1; // always sha256


    // FSM control signals for sha
    always @(posedge clk)
    begin
        op_reset <= 1'd0;
        op_init <= 1'd0;
        op_next  <= 1'd0;
        start_parallel <= 1'd0;
        case (state)
            STATE_COMPUTE: 
            begin
                op_reset <= 1'd1;
                op_init <= 1'd1;
                op_next <= 1'd0;
                op_block <= block1;
                start_parallel <= 1'd1;
            end
            STATE_COMPUTE_WAIT:
            begin
                op_reset <= 1'd1;
                op_init <= 1'd0;
                op_next <= 1'd0;
                op_block <= block1;
                start_parallel <= 1'd0;

            end
            STATE_NEXT:
            begin
                op_reset <= 1'd1;
                op_init <= 1'd0;
                op_next <= 1'd0;
                op_block <= block2;
                start_parallel <= 1'd0;
            end
            STATE_NEXT_WAIT:
            begin
                op_reset <= 1'd1;
                op_init <= 1'd0;
                op_next <= 1'd0;
                op_block <= block2;
                start_parallel <= 1'd0;
            end
        endcase
        case (next_state)
            STATE_NEXT:
            begin
                op_reset <= 1'd1;
                op_init <= 1'd0;
                op_next <= 1'd1;
                op_block <= block2;
            end
        endcase
    end
    
    // FSM for reading in data
    always@(posedge clk) begin
        case (state)
        STATE_RIN1 : begin
            block1[31:0] = rin1;
            block1[63:32] = rin2;
            block1[95:64] = rin3;
            block1[127:96] = rin4;
            block1[159:128] = rin5;
            block1[191:160] = rin6;
            block1[223:192] = rin7;
        end
        STATE_RIN2 : begin
            block1[255:224] = rin1;
            block1[287:256] = rin2;
            block1[319:288] = rin3;
            block1[351:320] = rin4;
            block1[383:352] = rin5;
            block1[415:384] = rin6;
            block1[447:416] = rin7;
        end
        STATE_RIN3 : begin
            block1[479:448] = rin1;
            block1[511:480] = rin2;
            block2[31:0] = rin3;
            block2[63:32] = rin4;
            block2[95:64] = rin5;
            block2[127:96] = rin6;
            block2[159:128] = rin7;
        end
        endcase
        case (state_parallel) 
        STATE_RIN4 : begin
            block2[191:160] = rin1;
            block2[223:192] = rin2;
            block2[255:224] = rin3;
            block2[287:256] = rin4;
            block2[319:288] = rin5;
            block2[351:320] = rin6;
            block2[383:352] = rin7;
        end
        STATE_RIN5 : begin
            block2[415:384] = rin1;
            block2[447:416] = rin2;
            block2[479:448] = rin3;
            block2[511:480] = rin4;
           
        end
        endcase 
    end

    // state logic
    always@(*) begin
        // defaults
        next_state   <= STATE_IDLE;

        // state defined logic
        case (state)
        // Wait in IDLE state till a compute command
        STATE_IDLE: begin
              next_state <= (isCmdComp) ? STATE_RIN1 : state;
        end
        
        
         STATE_RIN1: begin
         next_state <= (next_input2)? STATE_RIN2 : state;
         end
        
         STATE_RIN2: begin
         next_state <= (next_input3)? STATE_RIN3 : state;
         end
         
         STATE_RIN3: begin
         if (next_input4 & num_blocks) begin 
            next_state <= STATE_COMPUTE;
         end
         else if (next_input4 & !num_blocks) begin
            next_state <= STATE_COMPUTE;
         end
         else begin
            next_state <= state;
         end
         end
         
        // start computation of first block
        STATE_COMPUTE : begin
            next_state <= STATE_COMPUTE_WAIT;
        end

        // Compute the hash of the first block
        STATE_COMPUTE_WAIT : begin
            if (op_result_ready && num_blocks && block2_ready) next_state <= STATE_NEXT;
            else if (op_result_ready && !num_blocks) next_state <= STATE_WAIT;
            else next_state <= state;
        end
        
        // start computation if more than one block
        STATE_NEXT : begin
            next_state <= STATE_NEXT_WAIT;
        end
        
        STATE_NEXT_WAIT : begin
            next_state <= (op_result_ready) ? STATE_WAIT : state; //this should be expanded if more than 2 blocks
        end
        
        STATE_WAIT : begin
            next_state <= (next_output1)? STATE_ROUT1 : state;
        end

        
        STATE_ROUT1 : begin
            next_state <= (next_output2) ? STATE_ROUT2 : state;
        end

        
        STATE_ROUT2 : begin
            next_state <= (next_output3) ? STATE_DONE : state;
        end

        STATE_DONE : begin
            next_state <= (isCmdIdle) ? STATE_IDLE : state;
        end

        endcase
    end
    
    // state logic for parallel state
    always@(*) begin
        next_state_parallel <= 2'd0;
        block2_ready <= 1'd0;
        case (state_parallel)
            2'd0 : begin
                next_state_parallel <= (start_parallel & num_blocks)? STATE_RIN4 : state_parallel;
            end
            STATE_RIN4 : begin
                next_state_parallel <= (next_input5)? STATE_RIN5 : state_parallel;
            end
            STATE_RIN5 : begin
                next_state_parallel <= (next_input6)? 2'd0 : state_parallel;
                block2_ready <= (next_input6)? 1'd1 : 1'd0;
            end
        endcase
    end


    // Synchronous state transitions
    always@(posedge clk) begin
        state <= (~resetn) ? STATE_IDLE : next_state;
    end
    
    // Synchronous state transitions
    always@(posedge clk) begin
        state_parallel <= (~resetn) ? 2'd0 : next_state_parallel;
    end
    
    
    // call the sha256 implementation
    sha256_core SHA(clk, op_reset, op_init, op_next, op_mode, op_block, op_ready, op_result, op_result_ready);

    // registers to store value of hash
    reg[31:0] ro1, ro2, ro3, ro4, ro5, ro6, ro7, rbuffer;
    
   // FSM for output to c
   always@(posedge clk) begin
    case (state)
        STATE_COMPUTE_WAIT : begin
            ro1 = op_result[31:0];
            ro2 = op_result[63:32];
            ro3 = op_result[95:64];
            ro4 = op_result[127:96];
            ro5 = op_result[159:128];
            ro6 = op_result[191:160];
            ro7 = op_result[223:192];
            rbuffer = op_result[255:224];
        end
        STATE_NEXT_WAIT : begin
            ro1 = op_result[31:0];
            ro2 = op_result[63:32];
            ro3 = op_result[95:64];
            ro4 = op_result[127:96];
            ro5 = op_result[159:128];
            ro6 = op_result[191:160];
            ro7 = op_result[223:192];
            rbuffer = op_result[255:224];
        end

        STATE_ROUT2 : begin
        ro1 = rbuffer;
        end
    endcase 
   end
   
   // output to c code
   assign rout1 = ro1;
   assign rout2 = ro2;
   assign rout3 = ro3;
   assign rout4 = ro4;
   assign rout5 = ro5;
   assign rout6 = ro6;
   assign rout7 = ro7;


    // Status signals to the CPU
    wire isStateIdle = (state == STATE_IDLE);
    wire isStateDone = (state == STATE_DONE);
    wire op_finished = (state == STATE_WAIT);
    assign status = {28'd0,op_finished, dma_error, isStateIdle, isStateDone};

endmodule

