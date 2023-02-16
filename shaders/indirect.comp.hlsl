
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
	float3   Scale;
	uint64_t Buffer1;
	uint64_t Buffer2;
	uint     MeshIndex;
};

StructuredBuffer<mesh_offset> MeshOffsets : register(t0, space0);
StructuredBuffer<mesh_draw_command_data> MeshDrawCommandData : register(t1, space0);
RWStructuredBuffer<indirect_draw_command> IndirectDrawCommands : register(u0, space0);
RWStructuredBuffer<indirect_draw_indexed_command> IndirectDrawIndexedCommands : register(u1, space0);
RWStructuredBuffer<mesh_culling_common_input> MeshCullingCommonInput : register(u0, space1);


[numthreads(32, 1, 1)]
void main(uint3 ThreadGroupID : SV_GroupID, uint3 ThreadID : SV_GroupThreadID)
{
	uint DrawIndex = ThreadGroupID.x * 32 + ThreadID.x;
	if(DrawIndex == 0)
	{
		MeshCullingCommonInput[0].DrawCount = 0;
	}
	uint MeshIndex = MeshDrawCommandData[DrawIndex].MeshIndex;

	float3 SphereCenter = MeshOffsets[MeshIndex].BoundingSphere.Center.xyz * MeshDrawCommandData[DrawIndex].Scale + MeshDrawCommandData[DrawIndex].Translate.xyz;
	float SphereRadius = MeshOffsets[MeshIndex].BoundingSphere.Radius;

	bool IsVisible = true;
	IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[0].N, SphereCenter - MeshCullingCommonInput[0].Planes[0].P) > 0);
	IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[1].N, SphereCenter - MeshCullingCommonInput[0].Planes[1].P) > 0);
	IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[2].N, SphereCenter - MeshCullingCommonInput[0].Planes[2].P) > 0);
	IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[3].N, SphereCenter - MeshCullingCommonInput[0].Planes[3].P) > 0);
	IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[4].N, SphereCenter - MeshCullingCommonInput[0].Planes[4].P) > 0);
	IsVisible = IsVisible && (dot(MeshCullingCommonInput[0].Planes[5].N, SphereCenter - MeshCullingCommonInput[0].Planes[5].P) > 0);

	IsVisible = MeshCullingCommonInput[0].FrustrumCullingEnabled ? IsVisible : true;

	if(IsVisible)
	{
		uint VisibleIndex;
		InterlockedAdd(MeshCullingCommonInput[0].DrawCount, 1, VisibleIndex);

		IndirectDrawCommands[VisibleIndex].BufferPtr = MeshDrawCommandData[DrawIndex].Buffer1;
		IndirectDrawCommands[VisibleIndex].BufferPtr1 = MeshDrawCommandData[DrawIndex].Buffer2;
		IndirectDrawCommands[VisibleIndex].VertexDraw_VertexCountPerInstance = MeshOffsets[MeshIndex].VertexCount;
		IndirectDrawCommands[VisibleIndex].VertexDraw_DrawInstanceCount = 1;
		IndirectDrawCommands[VisibleIndex].VertexDraw_StartVertexLocation = MeshOffsets[MeshIndex].VertexOffset;
		IndirectDrawCommands[VisibleIndex].VertexDraw_StartInstanceLocation = 0;

		IndirectDrawIndexedCommands[VisibleIndex].BufferPtr = MeshDrawCommandData[DrawIndex].Buffer1;
		IndirectDrawIndexedCommands[VisibleIndex].BufferPtr1 = MeshDrawCommandData[DrawIndex].Buffer2;
		IndirectDrawIndexedCommands[VisibleIndex].IndexDraw_IndexCountPerInstance = MeshOffsets[MeshIndex].IndexCount;
		IndirectDrawIndexedCommands[VisibleIndex].IndexDraw_InstanceCount = 1;
		IndirectDrawIndexedCommands[VisibleIndex].IndexDraw_StartIndexLocation = MeshOffsets[MeshIndex].IndexOffset;
		IndirectDrawIndexedCommands[VisibleIndex].IndexDraw_BaseVertexLocation = 0;
		IndirectDrawIndexedCommands[VisibleIndex].IndexDraw_StartInstanceLocation = 0;
	}
}

