
struct vert_in
{
	float4 Pos : POSITION;
	float4 Nor : NORMAL;
	float4 Col : COLOR;
};

struct vert_out
{
	float4 Pos : SV_Position;
	float4 Col : COLOR;
};

[maxvertexcount(3)]
void main(triangle vert_in TrianIn[3], inout TriangleStream<vert_out> TrianOut)
{
	float3 CameraDir = float3(0, 0, -1);
	
	float4 AB = TrianIn[0].Pos - TrianIn[1].Pos;
	float4 CB = TrianIn[2].Pos - TrianIn[1].Pos;
	float3 NewNorm = cross(AB.xyz, CB.xyz);
	if(dot(NewNorm, CameraDir) < 0)
	{
		//TrianOut.RestartStrip();
		return;
	}

	for(uint VertIdx = 0;
		VertIdx < 3;
		++VertIdx)
	{
		vert_out Res;
		Res.Pos = TrianIn[VertIdx].Pos;
		Res.Col = TrianIn[VertIdx].Col;
		TrianOut.Append(Res);
	}
	//TrianOut.RestartStrip();
}

