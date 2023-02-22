
struct indirect_draw_command
{
	uint64_t BufferPtr;
	uint64_t BufferPtr1;

	uint VertexDraw_VertexCountPerInstance;
	uint VertexDraw_DrawInstanceCount;
	uint VertexDraw_StartVertexLocation;
	uint VertexDraw_StartInstanceLocation;
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
	uint DrawCount;
	uint FrustrumCullingEnabled;
	uint OcclusionCullingEnabled;
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
	float4   Translate;
	float    Scale;
	uint64_t Buffer1;
	uint64_t Buffer2;
	uint     MeshIndex;
	bool     IsVisible;
};

struct mesh_draw_command
{
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


[numthreads(32, 1, 1)]
void main(uint3 ThreadGroupID : SV_GroupID, uint3 ThreadID : SV_GroupThreadID)
{
	uint DrawIndex = ThreadGroupID.x * 32 + ThreadID.x;
	uint MeshIndex = MeshDrawCommandData[DrawIndex].MeshIndex;
	if(DrawIndex == 0)
	{
		IndirectDrawCommands.IncrementCounter();
		IndirectDrawCommands[MeshIndex].VertexDraw_DrawInstanceCount = 0;

		IndirectDrawIndexedCommands.IncrementCounter();
		IndirectDrawIndexedCommands[MeshIndex].IndexDraw_InstanceCount = 0;

		MeshCullingCommonInput[0].DrawCount = 0;
	}

	if(!MeshDrawCommandData[DrawIndex].IsVisible)
	{
		return;
	}

	float4x4 Proj = MeshCullingCommonInput[0].Proj;

	float3 SphereCenter = MeshOffsets[MeshIndex].BoundingSphere.Center.xyz * MeshDrawCommandData[DrawIndex].Scale + MeshDrawCommandData[DrawIndex].Translate.xyz;
	float SphereRadius = MeshOffsets[MeshIndex].BoundingSphere.Radius * MeshDrawCommandData[DrawIndex].Scale;

	float3 BoxMin = MeshOffsets[MeshIndex].AABB.Min.xyz * MeshDrawCommandData[DrawIndex].Scale + MeshDrawCommandData[DrawIndex].Translate.xyz;
	float3 BoxMax = MeshOffsets[MeshIndex].AABB.Max.xyz * MeshDrawCommandData[DrawIndex].Scale + MeshDrawCommandData[DrawIndex].Translate.xyz;
	BoxMin = mul(Proj, float4(BoxMin, 0)).xyz;
	BoxMax = mul(Proj, float4(BoxMax, 0)).xyz;

	bool IsVisible = true;
	// Frustum Culling
	if(MeshCullingCommonInput[0].FrustrumCullingEnabled)
	{
#if 0
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[0].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[1].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[2].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[3].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[4].N, SphereCenter) > -SphereRadius);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[5].N, SphereCenter) > -SphereRadius);
#else
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[0].N, SphereCenter - MeshCullingCommonInput[0].Planes[0].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[1].N, SphereCenter - MeshCullingCommonInput[0].Planes[1].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[2].N, SphereCenter - MeshCullingCommonInput[0].Planes[2].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[3].N, SphereCenter - MeshCullingCommonInput[0].Planes[3].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[4].N, SphereCenter - MeshCullingCommonInput[0].Planes[4].P) > 0);
		IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[5].N, SphereCenter - MeshCullingCommonInput[0].Planes[5].P) > 0);
#endif
	}

#if 0
	if(ThreadID.x == 31)
	{
		InterlockedAdd(IndirectDrawCommands[MeshIndex].VertexDraw_DrawInstanceCount, WaveActiveCountBits(IsVisible));
		InterlockedAdd(IndirectDrawIndexedCommands[MeshIndex].IndexDraw_InstanceCount, WaveActiveCountBits(IsVisible));
	}
#endif

	if(IsVisible && MeshDrawCommandData[DrawIndex].IsVisible)
	{
		uint DrawCommandIdx;
		indirect_draw_command IndirectDrawCommand;
		indirect_draw_indexed_command IndirectDrawIndexedCommand;

		InterlockedAdd(MeshCullingCommonInput[0].DrawCount, 1, DrawCommandIdx);

		IndirectDrawCommands[MeshIndex].BufferPtr = MeshDrawCommandData[DrawIndex].Buffer1;
		IndirectDrawCommands[MeshIndex].BufferPtr1 = MeshDrawCommandData[DrawIndex].Buffer2;
		IndirectDrawCommands[MeshIndex].VertexDraw_VertexCountPerInstance = MeshOffsets[MeshIndex].VertexCount;
		IndirectDrawCommands[MeshIndex].VertexDraw_StartVertexLocation = MeshOffsets[MeshIndex].VertexOffset;
		IndirectDrawCommands[MeshIndex].VertexDraw_StartInstanceLocation = 0;

		IndirectDrawIndexedCommands[MeshIndex].BufferPtr = MeshDrawCommandData[DrawIndex].Buffer1;
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
}

