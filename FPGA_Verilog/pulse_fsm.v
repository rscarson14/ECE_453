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
	
	