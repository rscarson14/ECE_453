`timescale 10ns/1ns

module DSM_tb();

	reg clk, rst_n, signal_in;

	wire signal_out;

	debounceSM DUT(
		.clk(clk),
		.rst_n(rst_n),
		.signal_in(signal_in),
		.signal_out(signal_out)
		);

	initial begin
		clk = 1'b0;
		rst_n = 1'b0;
		#8
		rst_n = 1'b1;
	end

	always begin
		#5 clk = ~clk;
	end

	initial begin
		#10
		signal_in = 1'b0;
		#20
		signal_in = 1'b1;
		#10
		signal_in = 1'b0;
		#10 
		signal_in = 1'b1;
		#20
		signal_in = 1'b0;
		#10
		signal_in = 1'b1;
		#60
		
		
		#200
		$finish;
	end
	
	initial begin
		$dumpfile("tb_dump.vcd");
		$dumpvars(0,DSM_tb);
	end

endmodule