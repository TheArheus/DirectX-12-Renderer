
struct depth_reduce_input
{
	uint MipLevel;
};

Texture2D<float> InputDepth : register(t0, space0);
//StructuredBuffer<depth_reduce_input> InputDepthData : register(t1, space0);
RWTexture2D<float> OutputDepth : register(u0, space0);
SamplerState DepthSampler : register(s0, space0);

[numthreads(32, 32, 1)]
void main(uint3 DispatchIndex : SV_DispatchThreadID)
{
	uint2 OutputDepthDims;
	OutputDepth.GetDimensions(OutputDepthDims.x, OutputDepthDims.y);
	//uint MipLevel = MipLevel == 0 ? 0 : InputDepthData[0].MipLevel;

	float2 InputDepthUV = (float2(DispatchIndex.xy) + float2(0.5, 0.5)) / float2(OutputDepthDims);

	float4 Depth = InputDepth.Gather(DepthSampler, InputDepthUV);

	OutputDepth[DispatchIndex.xy] = (Depth.x + Depth.y + Depth.z + Depth.w) / 4;
}
