
struct vert_out
{
	float4 Pos : SV_Position;
	float4 Col : COLOR;
};

vert_out main(float3 InPos : POSITION, float2 TexPos : TEXTCOORD, float3 Normal : NORMAL)
{	
	vert_out Out;
	Out.Pos = float4(InPos, 1.0);
	Out.Col = float4(Normal * 0.5 + float3(0.5, 0.5, 0.5), 1.0);
	return Out;
}

