#include "intrinsics.h"
#include "mesh.cpp"
#include "renderer_directx12.cpp"
#include "win32_window.cpp"

#include <random>

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
	mat4 Proj;
};


int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{	
	window DirectWindow(1240, 720, "3D Renderer");
	DirectWindow.InitGraphics();

	srand(512);
	double TargetFrameRate = 1.0 / 60 * 1000.0; // Frames Per Milliseconds

	mesh Geometries({"..\\assets\\kitten.obj"}, generate_aabb | generate_sphere);
	std::vector<mesh_draw_command_input> MeshDrawCommandData(DrawCount);
	std::vector<indirect_draw_command> IndirectDrawCommandVector(DrawCount);
	std::vector<indirect_draw_indexed_command> IndirectDrawIndexedCommandVector(DrawCount);

	memory_heap VertexHeap(MiB(16));
	memory_heap IndexHeap(MiB(16));

	u32 ZeroInt = 0;
	buffer ZeroBuffer(DirectWindow.Gfx, &ZeroInt, sizeof(u32));

	buffer VertexBuffer = VertexHeap.PushBuffer(DirectWindow.Gfx, Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex));
	buffer IndexBuffer = IndexHeap.PushBuffer(DirectWindow.Gfx, Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32));
	//DirectWindow.Gfx->PushIndexedVertexData(&VertexBuffer, &IndexBuffer, Geometries.Offsets);

	world_update WorldUpdate = {Identity(), PerspectiveInfFarZ(Pi<r64>/3, 1240, 720, 1)};
	buffer WorldUpdateBuffer(DirectWindow.Gfx, (void*)&WorldUpdate, sizeof(world_update), 256);

	mesh_comp_culling_common_input MeshCompCullingCommonData = {};
	MeshCompCullingCommonData.FrustrumCullingEnabled = true;
	MeshCompCullingCommonData.Proj = WorldUpdate.Proj;
	GeneratePlanes(MeshCompCullingCommonData.CullingPlanes, WorldUpdate.Proj, 1);
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

	u32 DepthMipLevel = 1 + floorf(logf(1240));
	u32 HiWidth  = 1240;
	u32 HiHeight = 720;
	std::vector<texture> DepthTextures(DepthMipLevel);
	std::vector<D3D12_SUBRESOURCE_DATA> DepthSubresources(DepthMipLevel);
	for(u32 DepthTextureIdx = 0;
		DepthTextureIdx < DepthMipLevel;
		++DepthTextureIdx)
	{
		HiWidth = HiWidth > 0 ? HiWidth >> 1 : 1;
		HiHeight = HiHeight > 0 ? HiHeight >> 1 : 1;
		texture DepthTexture(DirectWindow.Gfx, HiWidth, HiHeight, 1, DXGI_FORMAT_D32_FLOAT, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		DepthTextures[DepthTextureIdx] = DepthTexture;
	}

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

	auto SamplerDescriptorHeapHandle = DirectWindow.Gfx->CmpResourceHeap->GetCPUDescriptorHandleForHeapStart();
	SamplerDescriptorHeapHandle.ptr += 16 * DirectWindow.Gfx->ResourceHeapIncrement;
	D3D12_SHADER_RESOURCE_VIEW_DESC DepthTextureSRVDesc = {};
	DepthTextureSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	DepthTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	DepthTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	DepthTextureSRVDesc.Texture2D.MipLevels = 1;
	DepthTextureSRVDesc.Texture2D.PlaneSlice = 0;
	DepthTextureSRVDesc.Texture2D.MostDetailedMip = 0;
	DepthTextureSRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	DirectWindow.Gfx->Device->CreateShaderResourceView(DirectWindow.Gfx->DepthStencilBuffer.Handle.Get(), &DepthTextureSRVDesc, SamplerDescriptorHeapHandle);
	SamplerDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;

	for(u32 DepthTextureIdx = 0;
		DepthTextureIdx < DepthMipLevel;
		++DepthTextureIdx)
	{
		DirectWindow.Gfx->Device->CreateShaderResourceView(DepthTextures[DepthTextureIdx].Handle.Get(), &DepthTextureSRVDesc, SamplerDescriptorHeapHandle);
		SamplerDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC DepthTextureUAVDesc = {};
	DepthTextureUAVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	DepthTextureUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	DepthTextureUAVDesc.Texture2D.MipSlice = 0;
	DepthTextureUAVDesc.Texture2D.PlaneSlice = 0;
	for(u32 DepthTextureIdx = 0;
		DepthTextureIdx < DepthMipLevel;
		++DepthTextureIdx)
	{
		DirectWindow.Gfx->Device->CreateUnorderedAccessView(DepthTextures[DepthTextureIdx].Handle.Get(), nullptr, &DepthTextureUAVDesc, SamplerDescriptorHeapHandle);
		SamplerDescriptorHeapHandle.ptr += DirectWindow.Gfx->ResourceHeapIncrement;
	}

	shader_input GfxRootSignature;
	GfxRootSignature.PushConstantBuffer(0, 0)->
					 PushConstantBuffer(1, 0)->
					 Build(DirectWindow.Gfx->Device.Get(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	shader_input CmpRootSignature;
	CmpRootSignature.PushShaderResourceTable(0, 2, 0)->
					 PushUnorderedAccessTable(0, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE)->
					 PushUnorderedAccess(0, 1)->
					 Build(DirectWindow.Gfx->Device.Get());

	shader_input CmpReduceRootSignature;
	CmpReduceRootSignature.PushShaderResourceTable(0, 1, 0)->
						   PushUnorderedAccessTable(0, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE)->
						   PushSampler(0, 0)->
						   Build(DirectWindow.Gfx->Device.Get());

	indirect_command_signature DrawIndirectCommandSignature;
	DrawIndirectCommandSignature.PushConstantBuffer(0)->
								 PushConstantBuffer(1)->
								 PushCommandDraw()->
								 Build(DirectWindow.Gfx->Device.Get(), GfxRootSignature.Handle.Get());

	indirect_command_signature DrawIndexedIndirectCommandSignature;
	DrawIndexedIndirectCommandSignature.PushConstantBuffer(0)->
										PushConstantBuffer(1)->
										PushCommandDrawIndexed()->
										Build(DirectWindow.Gfx->Device.Get(), GfxRootSignature.Handle.Get());

	ComPtr<ID3D12PipelineState> GfxPipelineState = DirectWindow.Gfx->CreateGraphicsPipeline(GfxRootSignature.Handle.Get(), "..\\build\\mesh.vert.cso", "..\\build\\mesh.frag.cso");
	ComPtr<ID3D12PipelineState> CmpPipelineState = DirectWindow.Gfx->CreateComputePipeline(CmpRootSignature.Handle.Get(), "..\\build\\indirect.comp.cso");
	ComPtr<ID3D12PipelineState> CmpReducePipelineState = DirectWindow.Gfx->CreateComputePipeline(CmpReduceRootSignature.Handle.Get(), "..\\build\\depth_reduce.comp.cso");

	ID3D12GraphicsCommandList* CmpCommandList = DirectWindow.Gfx->CmpCommandList.Get();
	ID3D12GraphicsCommandList* GfxCommandList = DirectWindow.Gfx->GfxCommandList.Get();
	double TimeLast = window::GetTimestamp();
	double AvgCpuTime = 0.0;
	while(DirectWindow.IsRunning())
	{
		auto Result = DirectWindow.ProcessMessages();
		if(Result) return *Result;

		if(!DirectWindow.IsGfxPaused)
		{
			{
				auto DepthBufferHandle = DirectWindow.Gfx->CmpResourceHeap->GetGPUDescriptorHandleForHeapStart();
				DepthBufferHandle.ptr += 16 * DirectWindow.Gfx->ResourceHeapIncrement;
				auto InputDepthTextureHandle = DirectWindow.Gfx->CmpResourceHeap->GetGPUDescriptorHandleForHeapStart();
				InputDepthTextureHandle.ptr += 17 * DirectWindow.Gfx->ResourceHeapIncrement;
				auto OutputDepthTextureHandle = DirectWindow.Gfx->CmpResourceHeap->GetGPUDescriptorHandleForHeapStart();
				OutputDepthTextureHandle.ptr += 25 * DirectWindow.Gfx->ResourceHeapIncrement;

				DirectWindow.Gfx->CmpCommandAlloc->Reset();
				CmpCommandList->Reset(DirectWindow.Gfx->CmpCommandAlloc.Get(), CmpReducePipelineState.Get());

				CmpCommandList->SetComputeRootSignature(CmpReduceRootSignature.Handle.Get());
				CmpCommandList->SetPipelineState(CmpReducePipelineState.Get());

				ID3D12DescriptorHeap* Heaps[] = { DirectWindow.Gfx->CmpResourceHeap.Get() };
				CmpCommandList->SetDescriptorHeaps(1, Heaps);

				D3D12_RESOURCE_BARRIER DepthBufferBarrier[9] = {};
				DepthBufferBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				DepthBufferBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[0].Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				DepthBufferBarrier[2] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[1].Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				DepthBufferBarrier[3] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[2].Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				DepthBufferBarrier[4] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[3].Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				DepthBufferBarrier[5] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[4].Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				DepthBufferBarrier[6] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[5].Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				DepthBufferBarrier[7] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[6].Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				DepthBufferBarrier[8] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[7].Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				CmpCommandList->ResourceBarrier(9, DepthBufferBarrier);
				
				D3D12_GPU_DESCRIPTOR_HANDLE In  = {};
				D3D12_GPU_DESCRIPTOR_HANDLE Out = {};
				HiWidth  = 1240;
				HiHeight = 720;
				for(u32 DepthTextureIdx = 0;
					DepthTextureIdx < DepthMipLevel;
					++DepthTextureIdx)
				{
					HiWidth  = max(1, HiWidth >> 1);
					HiHeight = max(1, HiHeight >> 1);

					In.ptr  = DepthTextureIdx == 0 ? DepthBufferHandle.ptr : InputDepthTextureHandle.ptr + (DepthTextureIdx - 1) * DirectWindow.Gfx->ResourceHeapIncrement;
					Out.ptr = DepthTextureIdx == 0 ? OutputDepthTextureHandle.ptr : OutputDepthTextureHandle.ptr + DepthTextureIdx * DirectWindow.Gfx->ResourceHeapIncrement;

					CD3DX12_RESOURCE_BARRIER Barrier[2];
					if(DepthTextureIdx != 0)
					{
						Barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[DepthTextureIdx - 1].Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
						Barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[DepthTextureIdx].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
						CmpCommandList->ResourceBarrier(2, Barrier);
					}

					CmpCommandList->SetComputeRootDescriptorTable(0, In);
					CmpCommandList->SetComputeRootDescriptorTable(1, Out);
					CmpCommandList->Dispatch(floor((HiWidth + 31) / 32), floor((HiHeight + 31) / 32), 1);
				}

				DepthBufferBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[DepthMipLevel - 1].Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				CmpCommandList->ResourceBarrier(1, DepthBufferBarrier);

				CmpCommandList->Close();
				ID3D12CommandList* CmpCmdLists[] = { CmpCommandList };
				DirectWindow.Gfx->CmpCommandQueue->ExecuteCommandLists(_countof(CmpCmdLists), CmpCmdLists);
				DirectWindow.Gfx->Flush(DirectWindow.Gfx->CmpCommandQueue.Get());
			}

			{
				D3D12_GPU_DESCRIPTOR_HANDLE CmpResourceHeapHandle = DirectWindow.Gfx->CmpResourceHeap->GetGPUDescriptorHandleForHeapStart();

				DirectWindow.Gfx->CmpCommandAlloc->Reset();
				CmpCommandList->Reset(DirectWindow.Gfx->CmpCommandAlloc.Get(), CmpPipelineState.Get());

				CmpCommandList->SetComputeRootSignature(CmpRootSignature.Handle.Get());
				CmpCommandList->SetPipelineState(CmpPipelineState.Get());

				ID3D12DescriptorHeap* Heaps[] = { DirectWindow.Gfx->CmpResourceHeap.Get() };
				CmpCommandList->SetDescriptorHeaps(1, Heaps);

				CmpCommandList->SetComputeRootDescriptorTable(0, CmpResourceHeapHandle);
				CmpResourceHeapHandle.ptr += 2 * DirectWindow.Gfx->ResourceHeapIncrement;
				CmpCommandList->SetComputeRootDescriptorTable(1, CmpResourceHeapHandle);
				CmpCommandList->SetComputeRootUnorderedAccessView(2, MeshCommonCullingData.GpuPtr);

				CmpCommandList->CopyBufferRegion(IndirectDrawCommands.Handle.Get(), DrawCount * sizeof(indirect_draw_command), ZeroBuffer.Handle.Get(), 0, sizeof(u32));
				CmpCommandList->CopyBufferRegion(IndirectDrawIndexedCommands.Handle.Get(), DrawCount * sizeof(indirect_draw_indexed_command), ZeroBuffer.Handle.Get(), 0, sizeof(u32));

				D3D12_RESOURCE_BARRIER Barrier[] = 
				{
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawCommands.Handle.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawIndexedCommands.Handle.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
				};
				CmpCommandList->ResourceBarrier(2, Barrier);

				CmpCommandList->Dispatch(floor((DrawCount + 31) / 32), 1, 1);

				Barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawCommands.Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON),
				Barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawIndexedCommands.Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON),
				CmpCommandList->ResourceBarrier(2, Barrier);

				CmpCommandList->Close();
				ID3D12CommandList* CmpCmdLists[] = { CmpCommandList };
				DirectWindow.Gfx->CmpCommandQueue->ExecuteCommandLists(_countof(CmpCmdLists), CmpCmdLists);
				DirectWindow.Gfx->Flush(DirectWindow.Gfx->CmpCommandQueue.Get());
			}

			{
				DirectWindow.Gfx->GfxCommandAlloc->Reset();
				GfxCommandList->Reset(DirectWindow.Gfx->GfxCommandAlloc.Get(), GfxPipelineState.Get());

				GfxCommandList->SetGraphicsRootSignature(GfxRootSignature.Handle.Get());
				GfxCommandList->SetPipelineState(GfxPipelineState.Get());

				ID3D12DescriptorHeap* Heaps[] = {DirectWindow.Gfx->GfxResourceHeap.Get()};
				GfxCommandList->SetDescriptorHeaps(1, Heaps);

				CD3DX12_RESOURCE_BARRIER Barrier[11] = 
				{
					CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->BackBuffers[DirectWindow.Gfx->BackBufferIndex].Handle.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawIndexedCommands.Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
					CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
				};
				GfxCommandList->ResourceBarrier(3, Barrier);

				GfxCommandList->RSSetViewports(1, &DirectWindow.Gfx->Viewport);
				GfxCommandList->RSSetScissorRects(1, &DirectWindow.Gfx->Rect);

				CD3DX12_CPU_DESCRIPTOR_HANDLE RenderViewHandle(DirectWindow.Gfx->RtvHeap->GetCPUDescriptorHandleForHeapStart(), DirectWindow.Gfx->BackBufferIndex, DirectWindow.Gfx->RtvSize);
				D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView = DirectWindow.Gfx->DsvHeap->GetCPUDescriptorHandleForHeapStart();
				GfxCommandList->OMSetRenderTargets(1, &RenderViewHandle, true, &DepthStencilView);

				float ClearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
				GfxCommandList->ClearRenderTargetView(RenderViewHandle, ClearColor, 0, nullptr);
				GfxCommandList->ClearDepthStencilView(DirectWindow.Gfx->DsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);

				D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {VertexBuffer.GpuPtr, static_cast<u32>(VertexBuffer.Size), sizeof(vertex)};
				D3D12_INDEX_BUFFER_VIEW IndexBufferView = {IndexBuffer.GpuPtr, static_cast<u32>(IndexBuffer.Size), DXGI_FORMAT_R32_UINT};
				GfxCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				GfxCommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
				GfxCommandList->IASetIndexBuffer(&IndexBufferView);

				GfxCommandList->ExecuteIndirect(DrawIndexedIndirectCommandSignature.Handle.Get(), DrawCount, IndirectDrawIndexedCommands.Handle.Get(), 0, IndirectDrawIndexedCommands.Handle.Get(), DrawCount * sizeof(indirect_draw_indexed_command));

				Barrier[0]  = CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->BackBuffers[DirectWindow.Gfx->BackBufferIndex].Handle.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
				Barrier[1]  = CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawIndexedCommands.Handle.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COMMON),
				Barrier[2]  = CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);
				Barrier[3]  = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[0].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
				Barrier[4]  = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[1].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
				Barrier[5]  = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[2].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
				Barrier[6]  = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[3].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
				Barrier[7]  = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[4].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
				Barrier[8]  = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[5].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
				Barrier[9]  = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[6].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
				Barrier[10] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[7].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
				GfxCommandList->ResourceBarrier(11, Barrier);

				GfxCommandList->Close();
			}

			ID3D12CommandList* GfxCmdLists[] = { GfxCommandList };
			DirectWindow.Gfx->GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);
			DirectWindow.Gfx->Flush(DirectWindow.Gfx->GfxCommandQueue.Get());

			DirectWindow.Gfx->SwapChain->Present(0, 0);

			DirectWindow.Gfx->BackBufferIndex = DirectWindow.Gfx->SwapChain->GetCurrentBackBufferIndex();
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

