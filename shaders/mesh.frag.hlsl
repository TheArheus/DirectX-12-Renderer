
struct vert_res
{
	float4 Pos  : SV_Position;
	float4 Coord: POSITION0;
	float4 Norm : NORMAL;
	float4 Col  : COLOR;

	float4 CameraPos : POSITION1;
	float4 CameraDir : POSITION2;
};

struct material
{
	float4 LightEmmit;
	float  Specular;
	float  TextureIdx;
	float  NormalMapIdx;
	uint   Pad;
};

struct light_source
{
	float4 Pos;
	float4 Col;
};

float4 main(vert_res In) : SV_Target
{
	float3 Normal   = In.Norm.xyz;
	float3 Position = In.Coord.xyz;

	float3 CamPos   = In.CameraPos.xyz;
	float3 CamDir   = In.CameraDir.xyz;

	float3 LightPos = float3(0.45, 0.45, 0.45);
	float4 LightCol = float4(1.0, 1.0, 0.0, 0.6);
	float3 LightDir = (LightPos - Position).xyz;

	// Diffuse Calculations
	float Attenuation = 1.0 / dot(LightDir, LightDir);
	LightDir = normalize(LightDir);
	float AndleOfIncidence = max(dot(LightDir, Normal), 0.0);
	float3 Light = LightCol.xyz * LightCol.w * Attenuation;

	// Specular Calculations
	float3 HalfA = normalize(LightDir + CamDir);
	float3 BlinnTerm = clamp(dot(Normal, HalfA), 0, 1);
	BlinnTerm = pow(BlinnTerm, 2.0);

	float3 Ambient  = float3(1.0, 1.0, 1.0) * 0.3;
	float3 Diffuse  = Light * AndleOfIncidence;
	float3 Specular = Light * BlinnTerm;

	return float4(Ambient + Diffuse + Specular, 1.0) * In.Col;
}

