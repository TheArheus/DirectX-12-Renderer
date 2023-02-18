
Texture2D<float> InputDepth : register(t0, space0);
RWTexture2D<float> OutputDepth : register(u0, space0);
SamplerState DepthSampler : register(s0, space0);

[numthreads(32, 32, 1)]
void main(uint3 DispatchIndex : SV_DispatchThreadID)
{
	uint2 OutputDepthDims;
	OutputDepth.GetDimensions(OutputDepthDims.x, OutputDepthDims.y);

	float2 InputDepthUV = (float2(DispatchIndex.xy)) / float2(OutputDepthDims);

	float4 Depth = InputDepth.Gather(DepthSampler, InputDepthUV, 0);

	OutputDepth[DispatchIndex.xy] = max(max(Depth.x, Depth.y), max(Depth.z, Depth.w));
}
