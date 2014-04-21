module pulse_fsm(
	// Inputs
	input_vector, clk, rst_n, trigger,
	// Outputs
	output_vector
	);
	
	// Inputs
	input wire [5:0] input_vector;
	input wire clk, rst_n, trigger;
	
	// Outputs
	output reg [5:0] output_vector;
	
	wire [5:0] next_output_vector;
	
	always@(posedge clk, negedge rst_n)begin
			if(!rst_n)begin
				output_vector <= 6'b000000;
			end
			else begin
				output_vector <= next_output_vector;
			end
	end
	
	assign next_output_vector =  (trigger)	?	input_vector : 6'b000000;
	
	endmodule
	
	
	
	