#include "intrinsics.h"
#include "mesh.cpp"
#include "renderer_directx12.cpp"
#include "win32_window.cpp"

#include <random>

// NOTE: Create Command Signature

const u32 DrawCount = 1024*10;

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

struct mesh_draw_command_data
{
	vec4 Translate;
	vec3 Scale;
	u32  MeshIndex;
};

struct world_update
{
	mat4 View;
	mat4 Proj;
};

int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{	
	window DirectWindow(1240, 720, "3D Renderer");
	DirectWindow.InitGraphics();

	srand(512);
	double TargetFrameRate = 1.0 / 60 * 1000.0; // Frames Per Milliseconds

	mesh Geometries({"..\\assets\\kitten.obj", "..\\assets\\stanford-bunny.obj"}, generate_aabb | generate_sphere);
	std::vector<mesh_draw_command_data> MeshDrawCommandData(DrawCount);
	std::vector<indirect_draw_command> IndirectDrawCommandVector(DrawCount);
	std::vector<indirect_draw_indexed_command> IndirectDrawIndexedCommandVector(DrawCount);

	memory_heap VertexHeap(MiB(16));
	memory_heap IndexHeap(MiB(16));

	buffer VertexBuffer = VertexHeap.PushBuffer(DirectWindow.Gfx, Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex));
	buffer IndexBuffer = IndexHeap.PushBuffer(DirectWindow.Gfx, Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32));
	DirectWindow.Gfx->PushIndexedVertexData(&VertexBuffer, &IndexBuffer, Geometries.Offsets);

	u32 SceneRadius = 10;
	for(u32 DataIdx = 0;
		DataIdx < DrawCount;
		DataIdx++)
	{
		mesh_draw_command_data& CommandData = MeshDrawCommandData[DataIdx];
		CommandData.MeshIndex = rand() % Geometries.Offsets.size();
		CommandData.Scale[0] = 1.0f / 2;
		CommandData.Scale[1] = 1.0f / 2;
		CommandData.Scale[2] = 1.0f / 2;
		CommandData.Translate = vec4((float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
									 (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
									 (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 0.0f);
	}

	//world_update WorldUpdate = {Identity(), Perspective(Pi<r64>/3, 1240, 720, 1, 100)};
	world_update WorldUpdate = {Identity(), PerspectiveInfFarZ(Pi<r64>/3, 1240, 720, 1)};
	buffer WorldUpdateBuffer(DirectWindow.Gfx, (void*)&WorldUpdate, sizeof(world_update), 256);

	buffer GeometryOffsets(DirectWindow.Gfx, Geometries.Offsets.data(), Geometries.Offsets.size() * sizeof(mesh::offset));
	buffer MeshDrawCommandDataBuffer(DirectWindow.Gfx, MeshDrawCommandData.data(), sizeof(mesh_draw_command_data) * DrawCount);

	for(u32 DataIdx = 0;
		DataIdx < DrawCount;
		DataIdx++)
	{
		indirect_draw_command& IndirectDrawCommand = IndirectDrawCommandVector[DataIdx];
		indirect_draw_indexed_command& IndirectDrawIndexedCommand = IndirectDrawIndexedCommandVector[DataIdx];

		IndirectDrawCommand.Buffer = MeshDrawCommandDataBuffer.GpuPtr + sizeof(mesh_draw_command_data) * DataIdx;
		IndirectDrawCommand.Buffer1 = WorldUpdateBuffer.GpuPtr;
		IndirectDrawIndexedCommand.Buffer = MeshDrawCommandDataBuffer.GpuPtr + sizeof(mesh_draw_command_data) * DataIdx;
		IndirectDrawIndexedCommand.Buffer1 = WorldUpdateBuffer.GpuPtr;
	}

	buffer IndirectDrawCommands(DirectWindow.Gfx, IndirectDrawCommandVector.data(), sizeof(indirect_draw_command) * DrawCount, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	buffer IndirectDrawIndexedCommands(DirectWindow.Gfx, IndirectDrawIndexedCommandVector.data(), sizeof(indirect_draw_indexed_command) * DrawCount, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

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
	CmpSrvDesc1.Buffer.StructureByteStride = sizeof(mesh_draw_command_data);
	CmpSrvDesc1.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	DirectWindow.Gfx->Device->CreateShaderResourceView(MeshDrawCommandDataBuffer.Handle.Get(), &CmpSrvDesc1, CmpDescriptorHeapHandle);
	CmpDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;

	D3D12_UNORDERED_ACCESS_VIEW_DESC CmpUavDesc0 = {};
	CmpUavDesc0.Format = DXGI_FORMAT_UNKNOWN;
	CmpUavDesc0.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	CmpUavDesc0.Buffer.FirstElement = 0;
	CmpUavDesc0.Buffer.NumElements = DrawCount;
	CmpUavDesc0.Buffer.StructureByteStride = sizeof(indirect_draw_command);
	CmpUavDesc0.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	DirectWindow.Gfx->Device->CreateUnorderedAccessView(IndirectDrawCommands.Handle.Get(), nullptr, &CmpUavDesc0, CmpDescriptorHeapHandle);
	CmpDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;

	D3D12_UNORDERED_ACCESS_VIEW_DESC CmpUavDesc1 = {};
	CmpUavDesc1.Format = DXGI_FORMAT_UNKNOWN;
	CmpUavDesc1.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	CmpUavDesc1.Buffer.FirstElement = 0;
	CmpUavDesc1.Buffer.NumElements = DrawCount;
	CmpUavDesc1.Buffer.StructureByteStride = sizeof(indirect_draw_indexed_command);
	CmpUavDesc1.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	DirectWindow.Gfx->Device->CreateUnorderedAccessView(IndirectDrawIndexedCommands.Handle.Get(), nullptr, &CmpUavDesc1, CmpDescriptorHeapHandle);

	// Graphics Resources
	D3D12_ROOT_PARAMETER GfxRootParameter[2] = {};
	GfxRootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	GfxRootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	GfxRootParameter[0].Descriptor = {0, 0};
	GfxRootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	GfxRootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	GfxRootParameter[1].Descriptor = {1, 0};
	CD3DX12_ROOT_SIGNATURE_DESC GfxRootSignatureDesc;
	GfxRootSignatureDesc.Init(2, GfxRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> Signature;
	ComPtr<ID3DBlob> Error;
	ComPtr<ID3D12RootSignature> GfxRootSignature0;
	D3D12SerializeRootSignature(&GfxRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &Error);
	DirectWindow.Gfx->Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&GfxRootSignature0));

	// Compute Resources
	D3D12_ROOT_PARAMETER CmpRootParameter[4] = {};
	CmpRootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	CmpRootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	CmpRootParameter[0].Descriptor = {0, 0};
	CmpRootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	CmpRootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	CmpRootParameter[1].Descriptor = {1, 0};
	CmpRootParameter[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	CmpRootParameter[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	CmpRootParameter[2].Descriptor = {0, 0};
	CmpRootParameter[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	CmpRootParameter[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	CmpRootParameter[3].Descriptor = {1, 0};
	CD3DX12_ROOT_SIGNATURE_DESC CmpRootSignatureDesc;
	CmpRootSignatureDesc.Init(4, CmpRootParameter, 0, nullptr);

	ComPtr<ID3D12RootSignature> CmpRootSignature;
	D3D12SerializeRootSignature(&CmpRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &Error);
	DirectWindow.Gfx->Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&CmpRootSignature));

	ComPtr<ID3D12CommandSignature> DrawIndirectCommandSignature;
	D3D12_INDIRECT_ARGUMENT_DESC DrawArgumentDescs[3] = {};
	DrawArgumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	DrawArgumentDescs[0].ConstantBufferView.RootParameterIndex = 0;
	DrawArgumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	DrawArgumentDescs[1].ConstantBufferView.RootParameterIndex = 2;
	DrawArgumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
	D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc = {};
	CommandSignatureDesc.pArgumentDescs = DrawArgumentDescs;
	CommandSignatureDesc.NumArgumentDescs = 3;
	CommandSignatureDesc.ByteStride = sizeof(indirect_draw_command);
	DirectWindow.Gfx->Device->CreateCommandSignature(&CommandSignatureDesc, GfxRootSignature0.Get(), IID_PPV_ARGS(&DrawIndirectCommandSignature));

	ComPtr<ID3D12CommandSignature> DrawIndexedIndirectCommandSignature;
	D3D12_INDIRECT_ARGUMENT_DESC DrawIndexedArgumentDescs[3] = {};
	DrawIndexedArgumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	DrawIndexedArgumentDescs[0].ConstantBufferView.RootParameterIndex = 0;
	DrawIndexedArgumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	DrawIndexedArgumentDescs[1].ConstantBufferView.RootParameterIndex = 1;
	DrawIndexedArgumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	CommandSignatureDesc.pArgumentDescs = DrawIndexedArgumentDescs;
	CommandSignatureDesc.NumArgumentDescs = 3;
	CommandSignatureDesc.ByteStride = sizeof(indirect_draw_indexed_command);
	DirectWindow.Gfx->Device->CreateCommandSignature(&CommandSignatureDesc, GfxRootSignature0.Get(), IID_PPV_ARGS(&DrawIndexedIndirectCommandSignature));

	DirectWindow.Gfx->CreateGraphicsAndComputePipeline(GfxRootSignature0.Get(), CmpRootSignature.Get());
	double TimeLast = window::GetTimestamp();
	double AvgCpuTime = 0.0;
	while(DirectWindow.IsRunning())
	{
		auto Result = DirectWindow.ProcessMessages();
		if(Result) return *Result;

		if(!DirectWindow.IsGfxPaused)
		{
			DirectWindow.Gfx->BeginCompute(CmpRootSignature.Get(), GeometryOffsets, MeshDrawCommandDataBuffer, IndirectDrawCommands, IndirectDrawIndexedCommands);
			DirectWindow.Gfx->Dispatch(floor((DrawCount + 31) / 32), 1, 1);
			DirectWindow.Gfx->EndCompute(IndirectDrawCommands, IndirectDrawIndexedCommands);

			DirectWindow.Gfx->BeginRender(GfxRootSignature0.Get(), IndirectDrawIndexedCommands, WorldUpdateBuffer);
			DirectWindow.Gfx->DrawIndirect(DrawIndexedIndirectCommandSignature.Get(), VertexBuffer, IndexBuffer, DrawCount, IndirectDrawIndexedCommands);
			//DirectWindow.Gfx->DrawIndexed();
			DirectWindow.Gfx->EndRender(IndirectDrawIndexedCommands);
		}

		double TimeEnd = window::GetTimestamp();
		double TimeElapsed = (TimeEnd - TimeLast);

#if 0
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

