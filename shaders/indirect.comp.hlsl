
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

struct mesh_offset
{
	uint VertexOffset;
	uint VertexCount;

	uint IndexOffset;
	uint IndexCount;
};

struct mesh_draw_command_data
{
	float4 Translate;
	float3 Scale;
	uint   MeshIndex;
};

StructuredBuffer<mesh_offset> MeshOffsets : register(t0, space0);
StructuredBuffer<mesh_draw_command_data> MeshDrawCommandData : register(t1, space0);
RWStructuredBuffer<indirect_draw_command> IndirectDrawCommands : register(u0, space0);
RWStructuredBuffer<indirect_draw_indexed_command> IndirectDrawIndexedCommands : register(u1, space0);


[numthreads(32, 1, 1)]
void main(uint3 ThreadGroupID : SV_GroupID, uint3 ThreadID : SV_GroupThreadID)
{
	uint DrawIndex = ThreadGroupID.x * 32 + ThreadID.x;

	//IndirectDrawCommands[DrawIndex].BufferPtr = (uint64_t)MeshDrawCommandData;
	IndirectDrawCommands[DrawIndex].VertexDraw_VertexCountPerInstance = MeshOffsets[MeshDrawCommandData[DrawIndex].MeshIndex].VertexCount;
	IndirectDrawCommands[DrawIndex].VertexDraw_DrawInstanceCount = 1;
	IndirectDrawCommands[DrawIndex].VertexDraw_StartVertexLocation = MeshOffsets[MeshDrawCommandData[DrawIndex].MeshIndex].VertexOffset;
	IndirectDrawCommands[DrawIndex].VertexDraw_StartInstanceLocation = 0;

	//IndirectDrawIndexedCommands[DrawIndex].BufferPtr = (uint64_t)MeshDrawCommandData;
	IndirectDrawIndexedCommands[DrawIndex].IndexDraw_IndexCountPerInstance = MeshOffsets[MeshDrawCommandData[DrawIndex].MeshIndex].IndexCount;
	IndirectDrawIndexedCommands[DrawIndex].IndexDraw_InstanceCount = 1;
	IndirectDrawIndexedCommands[DrawIndex].IndexDraw_StartIndexLocation = MeshOffsets[MeshDrawCommandData[DrawIndex].MeshIndex].IndexOffset;
	IndirectDrawIndexedCommands[DrawIndex].IndexDraw_BaseVertexLocation = 0;
	IndirectDrawIndexedCommands[DrawIndex].IndexDraw_StartInstanceLocation = 0;
}

