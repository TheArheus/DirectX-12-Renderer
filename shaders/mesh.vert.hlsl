struct vert_in
{
	vector<float16_t, 4> Pos;
	vector<float, 2>	 TexPos;
	uint				 Normal;
};

StructuredBuffer<vert_in> VerticesIn : register(t0, space0);

struct vert_out
{
	float4 Pos : SV_Position;
	float4 Col : COLOR;
};

vert_out main(uint Index : SV_VertexID)
{	
	vert_in In = VerticesIn[Index];
	vert_out Out;
	float NormalX = ((In.Normal & 0xff000000) >> 24);
	float NormalY = ((In.Normal & 0x00ff0000) >> 16);
	float NormalZ = ((In.Normal & 0x0000ff00) >>  8);
	float3 Normal = float3(NormalX, NormalY, NormalZ) / 127.0 - 1.0;
	Out.Pos = In.Pos;
	Out.Col = float4(Normal * 0.5 + float3(0.5, 0.5, 0.5), 1.0);
	return Out;
}

