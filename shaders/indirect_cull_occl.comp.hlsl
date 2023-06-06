
struct indirect_draw_command
{
	uint64_t BufferPtr;
	uint64_t BufferPtr1;

	uint VertexDraw_VertexCountPerInstance;
	uint VertexDraw_DrawInstanceCount;
	uint VertexDraw_StartVertexLocation;
	uint VertexDraw_StartInstanceLocation;

	uint DrawCount;
};

struct indirect_draw_indexed_command
{
	uint64_t BufferPtr;
	uint64_t BufferPtr1;

	uint IndexDraw_IndexCountPerInstance;
	uint IndexDraw_InstanceCount;
	uint IndexDraw_StartIndexLocation;
	int  IndexDraw_BaseVertexLocation;
	uint IndexDraw_StartInstanceLocation;

	uint DrawCount;
};

struct material
{
	float4 LightEmmit;
	float  Specular;
	float  TextureIdx;
	float  NormalMapIdx;
	uint   Pad;
};

struct plane
{
	float3 P;
	uint   Pad0;
	float3 N;
	uint   Pad1;
};

struct mesh_culling_common_input
{
	plane Planes[6];
	uint  FrustrumCullingEnabled;
	uint  OcclusionCullingEnabled;
	float HiZWidth;
	float HiZHeight;
	row_major float4x4 Proj;
};

struct aabb
{
	float4 Min;
	float4 Max;
};

struct sphere
{
	float4 Center;
	float  Radius;
};

struct mesh_offset
{
	aabb AABB;
	sphere BoundingSphere;

	uint VertexOffset;
	uint VertexCount;

	uint IndexOffset;
	uint IndexCount;
};

struct mesh_draw_command_data
{
	material Mat;
	float4   Translate;
	float    Scale;
	uint64_t Buffer1;
	uint64_t Buffer2;
	uint     MeshIndex;
	bool     IsVisible;
};

struct mesh_draw_command
{
	material Mat;
	float4   Translate;
	float    Scale;
	float    Pad0;
	float    Pad1;
	float    Pad2;
};

StructuredBuffer<mesh_offset> MeshOffsets : register(t0, space0);
RWStructuredBuffer<mesh_draw_command_data> MeshDrawCommandData : register(u0, space0);
RWStructuredBuffer<indirect_draw_command> IndirectDrawCommands : register(u1, space0);
RWStructuredBuffer<indirect_draw_indexed_command> IndirectDrawIndexedCommands : register(u2, space0);
RWStructuredBuffer<mesh_draw_command> MeshDrawCommands : register(u3, space0);
RWStructuredBuffer<mesh_culling_common_input> MeshCullingCommonInput : register(u0, space1);
Texture2D HiZDepthTextures[] : register(t0, space1);
SamplerState DepthSampler : register(s0, space1);

[numthreads(32, 1, 1)]
void main(uint3 ThreadGroupID : SV_GroupID, uint3 ThreadID : SV_GroupThreadID)
{
	uint DrawIndex = ThreadGroupID.x * 32 + ThreadID.x;
	uint MeshIndex = MeshDrawCommandData[DrawIndex].MeshIndex;
	if(DrawIndex == 0)
	{
		IndirectDrawCommands[MeshIndex].VertexDraw_DrawInstanceCount = 0;
		IndirectDrawIndexedCommands[MeshIndex].IndexDraw_InstanceCount = 0;

		IndirectDrawCommands[MeshIndex].DrawCount = 0;
		IndirectDrawIndexedCommands[MeshIndex].DrawCount = 0;
	}

	float4x4 Proj = MeshCullingCommonInput[0].Proj;

	float  SphereRadius = MeshOffsets[MeshIndex].BoundingSphere.Radius * MeshDrawCommandData[DrawIndex].Scale;
	float3 SphereCenter = MeshOffsets[MeshIndex].BoundingSphere.Center.xyz * MeshDrawCommandData[DrawIndex].Scale + MeshDrawCommandData[DrawIndex].Translate.xyz;
	float4 ProjSphereTemp = mul(Proj, float4(SphereCenter, 1));
	float3 ProjSphereCenter = ProjSphereTemp.xyz / ProjSphereTemp.w;

	float3 BoxMin = MeshOffsets[MeshIndex].AABB.Min.xyz * MeshDrawCommandData[DrawIndex].Scale + MeshDrawCommandData[DrawIndex].Translate.xyz;
	float3 BoxMax = MeshOffsets[MeshIndex].AABB.Max.xyz * MeshDrawCommandData[DrawIndex].Scale + MeshDrawCommandData[DrawIndex].Translate.xyz;
	float4 ProjMinTemp = mul(Proj, float4(BoxMin, 1));
	float4 ProjMaxTemp = mul(Proj, float4(BoxMax, 1));
	float3 ProjBoxMin = ProjMinTemp.xyz / ProjMinTemp.w;
	float3 ProjBoxMax = ProjMaxTemp.xyz / ProjMaxTemp.w;

	bool IsVisible = MeshDrawCommandData[DrawIndex].IsVisible;
	// Frustum Culling
	if(MeshCullingCommonInput[0].FrustrumCullingEnabled)
	{
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[0].N, SphereCenter - MeshCullingCommonInput[0].Planes[0].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[1].N, SphereCenter - MeshCullingCommonInput[0].Planes[1].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[2].N, SphereCenter - MeshCullingCommonInput[0].Planes[2].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[3].N, SphereCenter - MeshCullingCommonInput[0].Planes[3].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[4].N, SphereCenter - MeshCullingCommonInput[0].Planes[4].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[5].N, SphereCenter - MeshCullingCommonInput[0].Planes[5].P) > 0);
	}

	// Occlusion Culling
	if(IsVisible && MeshCullingCommonInput[0].OcclusionCullingEnabled)
	{
		float3 BoxDims = (ProjBoxMax - ProjBoxMin);
		uint Lod = ceil(log2(max(BoxDims.x * MeshCullingCommonInput[0].HiZWidth, BoxDims.y * MeshCullingCommonInput[0].HiZHeight)));

		float4 Texel = HiZDepthTextures[Lod].Gather(DepthSampler, BoxDims.xy * 0.5);

		float ObjDepth = ProjBoxMax.z;
		IsVisible = IsVisible && (ObjDepth >= (min(min(Texel.x, Texel.y), min(Texel.z, Texel.w)) - 0.01));
	}

	if(IsVisible && !MeshDrawCommandData[DrawIndex].IsVisible)
	{
		uint DrawCommandIdx;
		InterlockedAdd(IndirectDrawCommands[MeshIndex].DrawCount, 1, DrawCommandIdx);
		InterlockedAdd(IndirectDrawIndexedCommands[MeshIndex].DrawCount, 1);

		IndirectDrawCommands[MeshIndex].BufferPtr  = MeshDrawCommandData[DrawIndex].Buffer1;
		IndirectDrawCommands[MeshIndex].BufferPtr1 = MeshDrawCommandData[DrawIndex].Buffer2;
		IndirectDrawCommands[MeshIndex].VertexDraw_VertexCountPerInstance = MeshOffsets[MeshIndex].VertexCount;
		IndirectDrawCommands[MeshIndex].VertexDraw_StartVertexLocation = MeshOffsets[MeshIndex].VertexOffset;
		IndirectDrawCommands[MeshIndex].VertexDraw_StartInstanceLocation = 0;

		IndirectDrawIndexedCommands[MeshIndex].BufferPtr  = MeshDrawCommandData[DrawIndex].Buffer1;
		IndirectDrawIndexedCommands[MeshIndex].BufferPtr1 = MeshDrawCommandData[DrawIndex].Buffer2;
		IndirectDrawIndexedCommands[MeshIndex].IndexDraw_IndexCountPerInstance = MeshOffsets[MeshIndex].IndexCount;
		IndirectDrawIndexedCommands[MeshIndex].IndexDraw_StartIndexLocation = MeshOffsets[MeshIndex].IndexOffset;
		IndirectDrawIndexedCommands[MeshIndex].IndexDraw_BaseVertexLocation = 0;
		IndirectDrawIndexedCommands[MeshIndex].IndexDraw_StartInstanceLocation = 0;

		MeshDrawCommands[DrawCommandIdx].Translate = MeshDrawCommandData[DrawIndex].Translate;
		MeshDrawCommands[DrawCommandIdx].Scale = MeshDrawCommandData[DrawIndex].Scale;

		InterlockedAdd(IndirectDrawCommands[MeshIndex].VertexDraw_DrawInstanceCount, 1);
		InterlockedAdd(IndirectDrawIndexedCommands[MeshIndex].IndexDraw_InstanceCount, 1);
	}
	MeshDrawCommandData[DrawIndex].IsVisible = IsVisible;
}

