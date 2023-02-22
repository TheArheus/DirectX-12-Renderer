struct vert_in
{
	vector<float16_t, 4> Pos    : POSITION;
	vector<float, 2>     TexPos : TEXTCOORD;
	uint                 Normal : NORMAL;
};

struct mesh_draw_command_data
{
	float4 Translate;
	float  Scale;
	uint   Pad0;
	uint   Pad1;
	uint   Pad2;
};

struct world_update
{
	row_major float4x4 View;
	row_major float4x4 Proj;
};

StructuredBuffer<mesh_draw_command_data> MeshDrawData : register(t0, space0);
ConstantBuffer<world_update> WorldUpdate : register(b0, space0);

struct vert_out
{
	float4 Pos : SV_Position;
	float4 Col : COLOR;
};

vert_out main(vert_in In, uint Index : SV_VertexID, uint InstanceID : SV_InstanceID)
{	
	vert_out Out;
	float NormalX = ((In.Normal & 0xff000000) >> 24);
	float NormalY = ((In.Normal & 0x00ff0000) >> 16);
	float NormalZ = ((In.Normal & 0x0000ff00) >>  8);
	float3 Normal = float3(NormalX, NormalY, NormalZ) / 127.0 - 1.0;
	Out.Pos = In.Pos * MeshDrawData[InstanceID].Scale + MeshDrawData[InstanceID].Translate;
	Out.Pos = mul(WorldUpdate.Proj, Out.Pos);
	Out.Pos = mul(WorldUpdate.View, Out.Pos);
	Out.Col = float4(Normal * 0.5 + float3(0.5, 0.5, 0.5), 1.0);
	return Out;
}

