
struct vert_res
{
	float4 Pos : SV_Position;
	float4 Col : COLOR;
};

float4 main(vert_res In) : SV_Target
{
	return In.Col;
}

