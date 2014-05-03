module test_bench();


reg [31:0] input_vector;
reg clk, rst_n;

wire [31:0] output_vector;

top_level DUT(
	.input_vector(input_vector),
	.clk(clk),
	.rst_n(rst_n),
	.output_vector(output_vector)
	);

initial begin
	rst_n = 0;
	#10;
	rst_n = 1;
	clk = 0;
end

always begin
	clk = ~clk;
	#5;
end

initial begin
	#25;
	input_vector = 31'h00000001;
	#500;
	$finish;
end

initial begin
	$dumpfile("tb_dump.vcd");
	$dumpvars(0,test_bench);
end

endmodule