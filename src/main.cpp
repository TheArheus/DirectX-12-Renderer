#include "intrinsics.h"
#include "mesh.cpp"
#include "renderer_directx12.cpp"
#include "win32_window.cpp"

#include <random>

// NOTE: Create Command Signature

const u32 DrawCount = 1024*50;

struct indirect_draw_command
{
	D3D12_GPU_VIRTUAL_ADDRESS Buffer;
	D3D12_GPU_VIRTUAL_ADDRESS Buffer1;
	D3D12_DRAW_ARGUMENTS DrawArg; // 4
};

struct indirect_draw_indexed_command
{
	D3D12_GPU_VIRTUAL_ADDRESS Buffer;
	D3D12_GPU_VIRTUAL_ADDRESS Buffer1;
	D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArg; // 5
};

struct mesh_draw_command_input
{
	vec4 Translate;
	vec3 Scale;
	uint64_t Buffer1;
	uint64_t Buffer2;
	u32  MeshIndex;
};

struct world_update
{
	mat4 View;
	mat4 Proj;
};

struct mesh_comp_culling_common_input
{
	plane CullingPlanes[6];
	u32 DrawCount;
	u32 FrustrumCullingEnabled;
};

// NOTE: This could be unified with descriptor heap allocator
class shader_input
{
	std::vector<D3D12_ROOT_PARAMETER1> Parameters;

	// TODO: Shader Visibility
public:
	shader_input* PushShaderResource(u32 Register, u32 Space, D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC)
	{
		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameter.Descriptor = {Register, Space, Flags};
		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushConstantBuffer(u32 Register, u32 Space, D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC)
	{
		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameter.Descriptor = {Register, Space, Flags};
		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushUnorderedAccess(u32 Register, u32 Space, D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC)
	{
		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameter.Descriptor = {Register, Space, Flags};
		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushShaderResourceTable(u32 Start, u32 Count, u32 Space, D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC)
	{
		// NOTE: don't like this solution. 
		// Maybe there could be something better?
		D3D12_DESCRIPTOR_RANGE1* ParameterRange = new D3D12_DESCRIPTOR_RANGE1;
		ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ParameterRange->BaseShaderRegister = Start;
		ParameterRange->NumDescriptors = Count;
		ParameterRange->RegisterSpace = Space;
		ParameterRange->Flags = Flags;
		ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.DescriptorTable = {1, ParameterRange};

		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushConstantBufferTable(u32 Start, u32 Count, u32 Space, D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC)
	{
		D3D12_DESCRIPTOR_RANGE1* ParameterRange = new D3D12_DESCRIPTOR_RANGE1;
		ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ParameterRange->BaseShaderRegister = Start;
		ParameterRange->NumDescriptors = Count;
		ParameterRange->RegisterSpace = Space;
		ParameterRange->Flags = Flags;
		ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.DescriptorTable = {1, ParameterRange};

		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushUnorderedAccessTable(u32 Start, u32 Count, u32 Space, D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC)
	{
		D3D12_DESCRIPTOR_RANGE1* ParameterRange = new D3D12_DESCRIPTOR_RANGE1;
		ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ParameterRange->BaseShaderRegister = Start;
		ParameterRange->NumDescriptors = Count;
		ParameterRange->RegisterSpace = Space;
		ParameterRange->Flags = Flags;
		ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.DescriptorTable = {1, ParameterRange};

		Parameters.push_back(Parameter);

		return this;
	}

	void Build(ID3D12Device1* Device, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
	{
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init_1_1(Parameters.size(), Parameters.data(), 0, nullptr, Flags);

		ComPtr<ID3DBlob> Signature;
		ComPtr<ID3DBlob> Error;
		HRESULT Res = D3DX12SerializeVersionedRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &Signature, &Error);
		Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&Handle));
	}

	ComPtr<ID3D12RootSignature> Handle;
};

class indirect_command_signature
{
	std::vector<D3D12_INDIRECT_ARGUMENT_DESC> Parameters;

public:

	indirect_command_signature* PushCommandDraw()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
		Parameters.push_back(Parameter);
		return this;
	}

	indirect_command_signature* PushCommandDrawIndexed()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
		Parameters.push_back(Parameter);
		return this;
	}

	indirect_command_signature* PushCommandDispatch()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
		Parameters.push_back(Parameter);
		return this;
	}	

	indirect_command_signature* PushCommandDispatchMesh()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
		Parameters.push_back(Parameter);
		return this;
	}	

	indirect_command_signature* PushCommandDispatchRays()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
		Parameters.push_back(Parameter);
		return this;
	}	

	indirect_command_signature* PushShaderResource(u32 RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
		Parameter.ShaderResourceView.RootParameterIndex = RootParameterIndex;
		Parameters.push_back(Parameter);
		return this;
	}

	indirect_command_signature* PushConstantBuffer(u32 RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
		Parameter.ConstantBufferView.RootParameterIndex = RootParameterIndex;
		Parameters.push_back(Parameter);
		return this;
	}

	indirect_command_signature* PushConstant(u32 RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		Parameter.Constant.RootParameterIndex = RootParameterIndex;
		Parameters.push_back(Parameter);
		return this;
	}

	indirect_command_signature* PushUnorderedAccess(u32 RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
		Parameter.UnorderedAccessView.RootParameterIndex = RootParameterIndex;
		Parameters.push_back(Parameter);
		return this;
	}

	void Build(ID3D12Device1* Device, ID3D12RootSignature* RootSignature, u32 IndirectCommandSize)
	{
		D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc = {};
		CommandSignatureDesc.pArgumentDescs = Parameters.data();
		CommandSignatureDesc.NumArgumentDescs = Parameters.size();
		CommandSignatureDesc.ByteStride = IndirectCommandSize;
		Device->CreateCommandSignature(&CommandSignatureDesc, RootSignature, IID_PPV_ARGS(&Handle));
	}

	ComPtr<ID3D12CommandSignature> Handle;
};

class descriptor_heap
{
	u32 Size;
	u32 Total;

public:
	descriptor_heap(ID3D12Device1* Device, u32 NumberOfDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.NumDescriptors = NumberOfDescriptors;
		HeapDesc.Type = Type;
		HeapDesc.Flags = Flags;
		Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Handle));
	}

	u32 AllocInc;
	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;

	ComPtr<ID3D12DescriptorHeap> Handle;
};

int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{	
	window DirectWindow(1240, 720, "3D Renderer");
	DirectWindow.InitGraphics();

	srand(512);
	double TargetFrameRate = 1.0 / 60 * 1000.0; // Frames Per Milliseconds

	mesh Geometries({"..\\assets\\kitten.obj", "..\\assets\\stanford-bunny.obj"}, generate_aabb | generate_sphere);
	std::vector<mesh_draw_command_input> MeshDrawCommandData(DrawCount);
	std::vector<indirect_draw_command> IndirectDrawCommandVector(DrawCount);
	std::vector<indirect_draw_indexed_command> IndirectDrawIndexedCommandVector(DrawCount);

	memory_heap VertexHeap(MiB(16));
	memory_heap IndexHeap(MiB(16));

	u32 ZeroInt = 0;
	buffer ZeroBuffer(DirectWindow.Gfx, &ZeroInt, sizeof(u32));

	buffer VertexBuffer = VertexHeap.PushBuffer(DirectWindow.Gfx, Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex));
	buffer IndexBuffer = IndexHeap.PushBuffer(DirectWindow.Gfx, Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32));
	DirectWindow.Gfx->PushIndexedVertexData(&VertexBuffer, &IndexBuffer, Geometries.Offsets);

	world_update WorldUpdate = {Identity(), PerspectiveInfFarZ(Pi<r64>/3, 1240, 720, 1)};
	buffer WorldUpdateBuffer(DirectWindow.Gfx, (void*)&WorldUpdate, sizeof(world_update), 256);

	mesh_comp_culling_common_input MeshCompCullingCommonData = {};
	MeshCompCullingCommonData.FrustrumCullingEnabled = true;
	GeneratePlanes(MeshCompCullingCommonData.CullingPlanes, WorldUpdate.Proj, 1, 100);
	buffer MeshCommonCullingData(DirectWindow.Gfx, &MeshCompCullingCommonData, sizeof(mesh_comp_culling_common_input), 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	buffer GeometryOffsets(DirectWindow.Gfx, Geometries.Offsets.data(), Geometries.Offsets.size() * sizeof(mesh::offset));
	buffer MeshDrawCommandDataBuffer(DirectWindow.Gfx, sizeof(mesh_draw_command_input) * DrawCount);

	u32 SceneRadius = 10;
	for(u32 DataIdx = 0;
		DataIdx < DrawCount;
		DataIdx++)
	{
		mesh_draw_command_input& CommandData = MeshDrawCommandData[DataIdx];
		CommandData.MeshIndex = rand() % Geometries.Offsets.size();
		CommandData.Buffer1 = MeshDrawCommandDataBuffer.GpuPtr + sizeof(mesh_draw_command_input) * DataIdx;
		CommandData.Buffer2 = WorldUpdateBuffer.GpuPtr;

		CommandData.Scale[0] = 1.0f / 2;
		CommandData.Scale[1] = 1.0f / 2;
		CommandData.Scale[2] = 1.0f / 2;
		CommandData.Translate = vec4((float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
									 (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
									 (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 0.0f);
	}

	MeshDrawCommandDataBuffer.Update(DirectWindow.Gfx, MeshDrawCommandData.data());

	buffer IndirectDrawCommands(DirectWindow.Gfx, IndirectDrawCommandVector.data(), sizeof(indirect_draw_command) * DrawCount + sizeof(u32), 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	buffer IndirectDrawIndexedCommands(DirectWindow.Gfx, IndirectDrawIndexedCommandVector.data(), sizeof(indirect_draw_indexed_command) * DrawCount + sizeof(u32), 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// Graphics Resources
	D3D12_CPU_DESCRIPTOR_HANDLE GfxDescriptorHeapHandle = DirectWindow.Gfx->GfxResourceHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_SHADER_RESOURCE_VIEW_DESC GfxSrvDesc0 = {};
	GfxSrvDesc0.Format = DXGI_FORMAT_UNKNOWN;
	GfxSrvDesc0.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	GfxSrvDesc0.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	GfxSrvDesc0.Buffer.FirstElement = 0;
	GfxSrvDesc0.Buffer.NumElements = DrawCount;
	GfxSrvDesc0.Buffer.StructureByteStride = sizeof(indirect_draw_indexed_command);
	GfxSrvDesc0.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	DirectWindow.Gfx->Device->CreateShaderResourceView(IndirectDrawIndexedCommands.Handle.Get(), &GfxSrvDesc0, GfxDescriptorHeapHandle);
	GfxDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;

	D3D12_CONSTANT_BUFFER_VIEW_DESC GfxCbvDesc0 = {};
	GfxCbvDesc0.BufferLocation = WorldUpdateBuffer.GpuPtr;
	GfxCbvDesc0.SizeInBytes = WorldUpdateBuffer.Size;
	DirectWindow.Gfx->Device->CreateConstantBufferView(&GfxCbvDesc0, GfxDescriptorHeapHandle);

	// Compute Resources
	D3D12_CPU_DESCRIPTOR_HANDLE CmpDescriptorHeapHandle = DirectWindow.Gfx->CmpResourceHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_SHADER_RESOURCE_VIEW_DESC CmpSrvDesc0 = {};
	CmpSrvDesc0.Format = DXGI_FORMAT_UNKNOWN;
	CmpSrvDesc0.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	CmpSrvDesc0.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	CmpSrvDesc0.Buffer.FirstElement = 0;
	CmpSrvDesc0.Buffer.NumElements = Geometries.Offsets.size();
	CmpSrvDesc0.Buffer.StructureByteStride = sizeof(mesh::offset);
	CmpSrvDesc0.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	DirectWindow.Gfx->Device->CreateShaderResourceView(GeometryOffsets.Handle.Get(), &CmpSrvDesc0, CmpDescriptorHeapHandle);
	CmpDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;

	D3D12_SHADER_RESOURCE_VIEW_DESC CmpSrvDesc1 = {};
	CmpSrvDesc1.Format = DXGI_FORMAT_UNKNOWN;
	CmpSrvDesc1.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	CmpSrvDesc1.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	CmpSrvDesc1.Buffer.FirstElement = 0;
	CmpSrvDesc1.Buffer.NumElements = MeshDrawCommandData.size();
	CmpSrvDesc1.Buffer.StructureByteStride = sizeof(mesh_draw_command_input);
	CmpSrvDesc1.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	DirectWindow.Gfx->Device->CreateShaderResourceView(MeshDrawCommandDataBuffer.Handle.Get(), &CmpSrvDesc1, CmpDescriptorHeapHandle);
	CmpDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;

	D3D12_UNORDERED_ACCESS_VIEW_DESC CmpUavDesc0 = {};
	CmpUavDesc0.Format = DXGI_FORMAT_UNKNOWN;
	CmpUavDesc0.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	CmpUavDesc0.Buffer.FirstElement = 0;
	CmpUavDesc0.Buffer.NumElements = DrawCount;
	CmpUavDesc0.Buffer.StructureByteStride = sizeof(indirect_draw_command);
	CmpUavDesc0.Buffer.CounterOffsetInBytes = DrawCount * sizeof(indirect_draw_command);
	CmpUavDesc0.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	DirectWindow.Gfx->Device->CreateUnorderedAccessView(IndirectDrawCommands.Handle.Get(), IndirectDrawCommands.Handle.Get(), &CmpUavDesc0, CmpDescriptorHeapHandle);
	CmpDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;

	D3D12_UNORDERED_ACCESS_VIEW_DESC CmpUavDesc1 = {};
	CmpUavDesc1.Format = DXGI_FORMAT_UNKNOWN;
	CmpUavDesc1.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	CmpUavDesc1.Buffer.FirstElement = 0;
	CmpUavDesc1.Buffer.NumElements = DrawCount;
	CmpUavDesc1.Buffer.StructureByteStride = sizeof(indirect_draw_indexed_command);
	CmpUavDesc1.Buffer.CounterOffsetInBytes = DrawCount * sizeof(indirect_draw_indexed_command);
	CmpUavDesc1.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	DirectWindow.Gfx->Device->CreateUnorderedAccessView(IndirectDrawIndexedCommands.Handle.Get(), IndirectDrawIndexedCommands.Handle.Get(), &CmpUavDesc1, CmpDescriptorHeapHandle);
	CmpDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;

	D3D12_UNORDERED_ACCESS_VIEW_DESC CmpUavDesc2 = {};
	CmpUavDesc2.Format = DXGI_FORMAT_UNKNOWN;
	CmpUavDesc2.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	CmpUavDesc2.Buffer.FirstElement = 0;
	CmpUavDesc2.Buffer.NumElements = 1;
	CmpUavDesc2.Buffer.StructureByteStride = sizeof(mesh_comp_culling_common_input);
	CmpUavDesc2.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	DirectWindow.Gfx->Device->CreateUnorderedAccessView(MeshCommonCullingData.Handle.Get(), nullptr, &CmpUavDesc2, CmpDescriptorHeapHandle);

	// Graphics Resources
	shader_input GfxRootSignature;
	GfxRootSignature.PushConstantBuffer(0, 0)->
					 PushConstantBuffer(1, 0)->
					 Build(DirectWindow.Gfx->Device.Get(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Compute Resources
	shader_input CmpRootSignature;
#if 0
	CmpRootSignature.PushShaderResource(0, 0)->
					 PushShaderResource(1, 0)->
					 PushUnorderedAccess(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE)->
					 PushUnorderedAccess(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE)->
					 PushUnorderedAccess(0, 1)->Build(DirectWindow.Gfx->Device.Get());
#else
	CmpRootSignature.PushShaderResourceTable(0, 2, 0);
	CmpRootSignature.PushUnorderedAccessTable(0, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
	CmpRootSignature.PushUnorderedAccess(0, 1);
	CmpRootSignature.Build(DirectWindow.Gfx->Device.Get());
#endif

	indirect_command_signature DrawIndirectCommandSignature;
	DrawIndirectCommandSignature.PushConstantBuffer(0)->
								 PushConstantBuffer(1)->
								 PushCommandDraw()->
								 Build(DirectWindow.Gfx->Device.Get(), GfxRootSignature.Handle.Get(), sizeof(indirect_draw_command));

	indirect_command_signature DrawIndexedIndirectCommandSignature;
	DrawIndexedIndirectCommandSignature.PushConstantBuffer(0)->
										PushConstantBuffer(1)->
										PushCommandDrawIndexed()->
										Build(DirectWindow.Gfx->Device.Get(), GfxRootSignature.Handle.Get(), sizeof(indirect_draw_indexed_command));

	DirectWindow.Gfx->CreateGraphicsAndComputePipeline(GfxRootSignature.Handle.Get(), CmpRootSignature.Handle.Get());
	double TimeLast = window::GetTimestamp();
	double AvgCpuTime = 0.0;
	while(DirectWindow.IsRunning())
	{
		auto Result = DirectWindow.ProcessMessages();
		if(Result) return *Result;

		if(!DirectWindow.IsGfxPaused)
		{
			DirectWindow.Gfx->BeginCompute(CmpRootSignature.Handle.Get(), GeometryOffsets, MeshDrawCommandDataBuffer, IndirectDrawCommands, IndirectDrawIndexedCommands, MeshCommonCullingData, ZeroBuffer, DrawCount * sizeof(indirect_draw_command), DrawCount * sizeof(indirect_draw_indexed_command));
			DirectWindow.Gfx->Dispatch(floor((DrawCount + 31) / 32), 1, 1);
			DirectWindow.Gfx->EndCompute(IndirectDrawCommands, IndirectDrawIndexedCommands);

			DirectWindow.Gfx->BeginRender(GfxRootSignature.Handle.Get(), IndirectDrawIndexedCommands, WorldUpdateBuffer);
			DirectWindow.Gfx->DrawIndirect(DrawIndexedIndirectCommandSignature.Handle.Get(), VertexBuffer, IndexBuffer, DrawCount, IndirectDrawIndexedCommands, DrawCount * sizeof(indirect_draw_indexed_command));
			//DirectWindow.Gfx->DrawIndexed();
			DirectWindow.Gfx->EndRender(IndirectDrawIndexedCommands);
		}

		double TimeEnd = window::GetTimestamp();
		double TimeElapsed = (TimeEnd - TimeLast);

#if 1
		if(TimeElapsed < TargetFrameRate)
		{
			while(TimeElapsed < TargetFrameRate)
			{
				double TimeToSleep = TargetFrameRate - TimeElapsed;
				if (TimeToSleep > 0) 
				{
					Sleep(TimeToSleep);
				}
				TimeEnd = window::GetTimestamp();
				TimeElapsed = TimeEnd - TimeLast;
			}
		}
#endif

		TimeLast = TimeEnd;
		AvgCpuTime = 0.75 * AvgCpuTime + TimeElapsed * 0.25;

		std::string Title = "Frame " + std::to_string(AvgCpuTime) + "ms, " + std::to_string(1.0 / AvgCpuTime * 1000.0) + "fps";
		DirectWindow.SetTitle(Title);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	return WinMain(0, 0, argv[0], SW_SHOWNORMAL);
}

