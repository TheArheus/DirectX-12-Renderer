
struct vertex_input
{
	float4 Pos : SV_Position;
	float4 Col : COLOR;
};


float4 main(vertex_input In) : SV_Target
{
	return In.Col;
}
