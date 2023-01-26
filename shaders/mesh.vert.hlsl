
struct vertex_output
{
	float4 Pos : SV_Position;
	float4 Col : COLOR;
};

vertex_output main(float4 Position : POSITION, float4 Color : COLOR)
{	
	vertex_output Out;
	Out.Pos = Position;
	Out.Col = Color;
	return Out;
}
