module top_level(input_vector, output_vector, clk, rst_n);

	//inputs
	input [31:0] input_vector;
	input clk, rst_n;
	
	// Outputs
	output [31:0] output_vector;
	
	wire trigger;
	
	pulse_fsm P_FSM(
		.clk(clk),
		.rst_n(rst_n),
		.trigger(trigger),
		.input_vector(input_vector[5:0]),
		.output_vector(output_vector[5:0])
		);
		
	counter COUNT(
		.clk(clk),
		.rst_n(rst_n),
		.trigger(trigger)
		);
		
	assign output_vector[31:6] = 26'h0000000;

endmodule

