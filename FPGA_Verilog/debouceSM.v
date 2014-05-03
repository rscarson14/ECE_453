module debounceSM(
	//Inputs
	signal_in, rst_n, clk,
	//Outputs
	signal_out);
	
	//Inputs
	input wire signal_in, rst_n, clk;
	
	//Outputs
	output wire signal_out;
	
	localparam target_count = 4'h5;
	
	localparam state_low = 2'b00;
	localparam state_posedge = 2'b01;
	localparam state_negedge = 2'b10;
	localparam state_high = 2'b11;
	
	reg [1:0] current_state;
	reg [3:0] count;
	
	wire [1:0] next_state;
	wire reset_count;
	
	always@(posedge clk, negedge rst_n)begin
		if(~rst_n)begin
			current_state <= 2'b00;
			count <= 4'b0000;
		end
		else begin
			current_state <= next_state;
			count <= next_count;
		end
	end
	
	assign next_count = (reset_count) ? 4'b0000 : count+1;
	
	always@(current_state, signal_in, count)begin
		next_state = state_low;
		signal_out = 1'b0;
		reset_count = 1'b1;
		
		case(current_state) begin
			
			state_low : begin
				signal_out = 1'b0;
				if(signal_in)begin
					reset_count = 1'b0;
					next_state = state_posedge;
				end
				else begin
					reset_count = 1'b1;
					next_state = state_low;
				end
			end
			
			
			state_posedge : begin
				signal_out = 1'b0;
				
				if(signal_in)begin
					reset_count = 1'b0;
				end
				else begin
					reset_count = 1'b1;
				end
				
				if(count == target_count && signal_in)begin
					next_state = state_high;
				end
				else if(signal_in)begin
					next_state = state_posedge;
				end
				else
					next_state = state_low;
				end
			end
			
			
			state_high : begin
				signal_out = 1'b1;
				if(~signal_in)begin
					reset_count = 1'b0;
					next_state = state_negedge;
				end
				else begin
					reset_count = 1'b1;
					next_state = state_high;
				end
			end
			

			state_negedge : begin
				signal_out = 1'b1;
				
				if(~signal_in)begin
					reset_count = 1'b0;
				end
				else begin
					reset_count = 1'b1;
				end
				
				if(count == target_count && ~signal_in)begin
					next_state = state_low;
				end
				else if(~signal_in)begin
					next_state = state_negedge;
				end
				else
					next_state = state_high;
				end
			end
	end

endmodule