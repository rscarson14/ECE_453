module counter(
	// Inputs
	clk, rst_n,
	// Outputs
	trigger
	);
	
	// Inputs
	input wire clk, rst_n;
	
	//Outputs
	output wire trigger;
	
	reg [15:0] current_count;
	wire [15:0] inc;
	
	always @(posedge clk, negedge rst_n) begin
		if(!rst_n) begin
			current_count <= 16'h0000;
		end 
		else
			current_count <= inc;
	end
	
	assign inc = current_count+1;
	assign trigger = (current_count == target_count) ? 1'b1 : 1'b0;

endmodule;