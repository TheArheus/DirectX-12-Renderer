#include "intrinsics.h"
#include "mesh.cpp"
#include "renderer_directx12.cpp"
#include "win32_window.cpp"

#include <random>

const u32 DrawCount = 1024;

struct indirect_draw_command
{
	D3D12_GPU_VIRTUAL_ADDRESS Buffer;
	D3D12_GPU_VIRTUAL_ADDRESS Buffer1;
	D3D12_DRAW_ARGUMENTS DrawArg; // 4
	u32 DrawCount;
};

struct indirect_draw_indexed_command
{
	D3D12_GPU_VIRTUAL_ADDRESS Buffer;
	D3D12_GPU_VIRTUAL_ADDRESS Buffer1;
	D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArg; // 5
	u32 DrawCount;
};

struct mesh_draw_command_input
{
	mesh::material Mat;
	vec4 Translate;
	float Scale;
	uint64_t Buffer1;
	uint64_t Buffer2;
	u32  MeshIndex;
	bool IsVisible;
};

struct mesh_draw_command
{
	mesh::material Mat;
	vec4  Translate;
	float Scale;
	float Pad0;
	float Pad1;
	float Pad2;
};

struct global_world_data 
{
	mat4 View;
	mat4 Proj;
	vec4 CameraPos;
	vec4 CameraDir;
	u32  ColorSourceCount;
};

struct light_source
{
	vec4 Pos;
	vec4 Col;
};

struct mesh_comp_culling_common_input
{
	plane CullingPlanes[6];
	u32   FrustrumCullingEnabled;
	u32   OcclusionCullingEnabled;
	float HiZWidth;
	float HiZHeight;
	mat4  Proj;
};

std::vector<light_source> Lights;

class geometry_instance
{
	u32 ID;

public:
	geometry_instance() = default;

	void AddInstance(vec3 Translate, vec3 Scale);
};

int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{	
	window DirectWindow(1240, 720, "3D Renderer");
	DirectWindow.InitGraphics();

	vec3 CameraPos{}, ViewDir(0, 0, 1);

	srand(128);
	double TargetFrameRate = 1.0 / 60 * 1000.0; // Frames Per Milliseconds

	mesh Geometries({"..\\assets\\cube.obj"}, generate_aabb | generate_sphere);
	std::vector<mesh_draw_command_input> MeshDrawCommandData(DrawCount);
	std::vector<indirect_draw_command> IndirectDrawCommandVector(DrawCount);
	std::vector<indirect_draw_indexed_command> IndirectDrawIndexedCommandVector(DrawCount);

	u32 GeometriesCounterBufferData = Geometries.Offsets.size();
	buffer GeometriesCounterBuffer(DirectWindow.Gfx, &GeometriesCounterBufferData, sizeof(u32));

	memory_heap VertexHeap(DirectWindow.Gfx, MiB(16));
	memory_heap IndexHeap(DirectWindow.Gfx, MiB(16));

	buffer VertexBuffer = VertexHeap.PushBuffer(DirectWindow.Gfx, Geometries.Vertices.data(), Geometries.Vertices.size() * sizeof(vertex));
	buffer IndexBuffer = IndexHeap.PushBuffer(DirectWindow.Gfx, Geometries.VertexIndices.data(), Geometries.VertexIndices.size() * sizeof(u32));

	global_world_data WorldUpdate = {Identity(), PerspectiveInfFarZ(Pi<r64>/3, 1240, 720, 1)};
	buffer WorldUpdateBuffer(DirectWindow.Gfx, (void*)&WorldUpdate, sizeof(global_world_data), false, 256);

	mesh_comp_culling_common_input MeshCompCullingCommonData = {};
	MeshCompCullingCommonData.FrustrumCullingEnabled = false;
	MeshCompCullingCommonData.OcclusionCullingEnabled = false;
	MeshCompCullingCommonData.Proj = WorldUpdate.Proj;
	GeneratePlanes(MeshCompCullingCommonData.CullingPlanes, WorldUpdate.Proj, 1);

	buffer GeometryOffsets(DirectWindow.Gfx, Geometries.Offsets.data(), Geometries.Offsets.size() * sizeof(mesh::offset));
	buffer MeshDrawCommandDataBuffer(DirectWindow.Gfx, sizeof(mesh_draw_command_input) * DrawCount, false, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	buffer MeshDrawCommandBuffer(DirectWindow.Gfx, sizeof(mesh_draw_command) * DrawCount, false, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	u32 SceneRadius = 5;
	for(u32 DataIdx = 0;
		DataIdx < DrawCount;
		DataIdx++)
	{
		mesh_draw_command_input& CommandData = MeshDrawCommandData[DataIdx];
		CommandData.MeshIndex = rand() % Geometries.Offsets.size();
		CommandData.Buffer1 = MeshDrawCommandBuffer.GpuPtr;
		CommandData.Buffer2 = WorldUpdateBuffer.GpuPtr;

		CommandData.IsVisible = true;
		CommandData.Scale = 1.0f / 10;
		CommandData.Translate = vec4((float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
									 (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 
									 (float(rand()) / RAND_MAX) * 2 * SceneRadius - SceneRadius, 0.0f);
	}

	MeshDrawCommandDataBuffer.Update(DirectWindow.Gfx, MeshDrawCommandData.data());

	u32 HiWidth  = PreviousPowerOfTwo(1240);
	u32 HiHeight = PreviousPowerOfTwo(720);
	u32 DepthMipLevel = GetImageMipLevels(HiWidth, HiHeight);
	std::vector<texture> DepthTextures(DepthMipLevel);
	for(u32 DepthIdx = 0;
		DepthIdx < DepthMipLevel;
		++DepthIdx)
	{
		DepthTextures[DepthIdx] = texture(DirectWindow.Gfx, max(1, HiWidth >> DepthIdx), max(1, HiHeight >> DepthIdx), 1, DXGI_FORMAT_R32_FLOAT, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	}

	buffer IndirectDrawCommands(DirectWindow.Gfx, IndirectDrawCommandVector.data(), sizeof(indirect_draw_command) * DrawCount, true, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	buffer IndirectDrawIndexedCommands(DirectWindow.Gfx, IndirectDrawIndexedCommandVector.data(), sizeof(indirect_draw_indexed_command) * DrawCount, true, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	MeshCompCullingCommonData.HiZWidth  = float(HiWidth);
	MeshCompCullingCommonData.HiZHeight = float(HiHeight);
	buffer MeshCommonCullingData(DirectWindow.Gfx, &MeshCompCullingCommonData, sizeof(mesh_comp_culling_common_input), false, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// Graphics Resources
	heap_alloc GfxDescriptorHeapHandle = DirectWindow.Gfx->ResourceHeap.Allocate(2);
	GfxDescriptorHeapHandle.PushShaderResourceBufferView(DirectWindow.Gfx, IndirectDrawIndexedCommands, sizeof(indirect_draw_indexed_command), DrawCount);
	GfxDescriptorHeapHandle.PushConstantBufferView(DirectWindow.Gfx, WorldUpdateBuffer);

	// Compute Resources
	heap_alloc MeshOffsetsTable = DirectWindow.Gfx->ResourceHeap.Allocate(1);
	MeshOffsetsTable.PushShaderResourceBufferView(DirectWindow.Gfx, GeometryOffsets, sizeof(mesh::offset), Geometries.Offsets.size());

	heap_alloc IndirectCommandsDataTable = DirectWindow.Gfx->ResourceHeap.Allocate(4);
	IndirectCommandsDataTable.PushUnorderedAccessBufferView(DirectWindow.Gfx, MeshDrawCommandDataBuffer, sizeof(mesh_draw_command_input), MeshDrawCommandData.size());
	IndirectCommandsDataTable.PushUnorderedAccessBufferViewWithCounter(DirectWindow.Gfx, IndirectDrawCommands, IndirectDrawCommands, sizeof(indirect_draw_command), DrawCount);
	IndirectCommandsDataTable.PushUnorderedAccessBufferViewWithCounter(DirectWindow.Gfx, IndirectDrawIndexedCommands, IndirectDrawIndexedCommands, sizeof(indirect_draw_indexed_command), DrawCount);
	IndirectCommandsDataTable.PushUnorderedAccessBufferView(DirectWindow.Gfx, MeshDrawCommandBuffer, sizeof(mesh_draw_command), DrawCount);

	heap_alloc CullingCommonInputTable = DirectWindow.Gfx->ResourceHeap.Allocate(1);
	CullingCommonInputTable.PushUnorderedAccessBufferView(DirectWindow.Gfx, MeshCommonCullingData, sizeof(mesh_comp_culling_common_input));

	heap_alloc DepthBufferTable = DirectWindow.Gfx->ResourceHeap.Allocate(1);
	DepthBufferTable.PushShaderResourceTexture2DView(DirectWindow.Gfx, DirectWindow.Gfx->DepthStencilBuffer);

	heap_alloc HiZInputTable  = DirectWindow.Gfx->ResourceHeap.Allocate(DepthMipLevel);
	heap_alloc HiZOutputTable = DirectWindow.Gfx->ResourceHeap.Allocate(DepthMipLevel);
	for(u32 DepthIdx = 0;
		DepthIdx < DepthMipLevel;
		++DepthIdx)
	{
		HiZInputTable.PushShaderResourceTexture2DView(DirectWindow.Gfx, DepthTextures[DepthIdx]);
	}
	for(u32 DepthIdx = 0;
		DepthIdx < DepthMipLevel;
		++DepthIdx)
	{
		HiZOutputTable.PushUnorderedAccessTexture2DView(DirectWindow.Gfx, DepthTextures[DepthIdx]);
	}

	shader_input GfxRootSignature;
	GfxRootSignature.PushShaderResource(0, 0)->
					 PushConstantBuffer(0, 0)->
					 Build(DirectWindow.Gfx->Device.Get(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	shader_input CmpIndirectFrustRootSignature;
	CmpIndirectFrustRootSignature.PushShaderResourceTable(0, 1, 0)-> // Mesh Offsets
					 PushUnorderedAccessTable(0, 4, 0)->			 // Common Data, Indirect Commands
					 PushUnorderedAccess(0, 1)->					 // Common Input
					 Build(DirectWindow.Gfx->Device.Get());

	shader_input CmpIndirectOcclRootSignature;
	CmpIndirectOcclRootSignature.PushShaderResourceTable(0, 1, 0)-> // Mesh Offsets
					 PushUnorderedAccessTable(0, 4, 0)->			// Common Data
					 PushUnorderedAccess(0, 1)->				    // Common Input
					 PushShaderResourceTable(0, DepthMipLevel, 1)-> // Hierarchy-Z depth texture
					 PushSampler(0, 1)->
					 Build(DirectWindow.Gfx->Device.Get());

	shader_input CmpReduceRootSignature;
	CmpReduceRootSignature.PushShaderResourceTable(0, 1, 0)->  // Input  Texture
						   PushUnorderedAccessTable(0, 1, 0)-> // Output Texture
						   PushSampler(0, 0)->
						   Build(DirectWindow.Gfx->Device.Get());

	indirect_command_signature DrawIndirectCommandSignature;
	DrawIndirectCommandSignature.PushShaderResource(0)->
								 PushConstantBuffer(1)->
								 PushCommandDraw()->
								 Build(DirectWindow.Gfx->Device.Get(), GfxRootSignature.Handle.Get());

	indirect_command_signature DrawIndexedIndirectCommandSignature;
	DrawIndexedIndirectCommandSignature.PushShaderResource(0)->
										PushConstantBuffer(1)->
										PushCommandDrawIndexed()->
										Build(DirectWindow.Gfx->Device.Get(), GfxRootSignature.Handle.Get());

	render_context GfxContext(DirectWindow.Gfx, GfxRootSignature, VertexBuffer, IndexBuffer, {"..\\build\\mesh.vert.cso", "..\\build\\mesh.frag.cso"}, DrawIndexedIndirectCommandSignature);
	compute_context FrustCullingContext(DirectWindow.Gfx, CmpIndirectFrustRootSignature, "..\\build\\indirect_cull_frust.comp.cso");
	compute_context OcclCullingContext(DirectWindow.Gfx, CmpIndirectOcclRootSignature, "..\\build\\indirect_cull_occl.comp.cso");
	compute_context ReduceContext(DirectWindow.Gfx, CmpReduceRootSignature, "..\\build\\depth_reduce.comp.cso");

	heap_alloc RtvHeapMap = DirectWindow.Gfx->RtvHeap.Allocate(2);
	heap_alloc DsvHeapMap = DirectWindow.Gfx->DsvHeap.Allocate(1);
	DirectWindow.Gfx->RtvHeap.Reset();
	DirectWindow.Gfx->DsvHeap.Reset();

	vec3 UpVector(0, 1, 0);
	double TimeLast = window::GetTimestamp();
	double AvgCpuTime = 0.0;
	while(DirectWindow.IsRunning())
	{
		auto Result = DirectWindow.ProcessMessages();
		if(Result) return *Result;
		if(DirectWindow.Width != DirectWindow.Gfx->Width || DirectWindow.Height != DirectWindow.Gfx->Height)
		{
			DirectWindow.Gfx->RecreateSwapchain(DirectWindow.Width, DirectWindow.Height);

			HiWidth  = PreviousPowerOfTwo(DirectWindow.Width);
			HiHeight = PreviousPowerOfTwo(DirectWindow.Height);
			DepthMipLevel = GetImageMipLevels(HiWidth, HiHeight);
			DepthTextures = std::vector<texture>(DepthMipLevel);
			for(u32 DepthIdx = 0;
				DepthIdx < DepthMipLevel;
				++DepthIdx)
			{
				DepthTextures[DepthIdx] = texture(DirectWindow.Gfx, max(1, HiWidth >> DepthIdx), max(1, HiHeight >> DepthIdx), 1, DXGI_FORMAT_R32_FLOAT, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			}
		}

		WorldUpdate = 
		{
			LookAt(CameraPos, CameraPos + ViewDir, UpVector), 
			PerspectiveInfFarZ(Pi<r64>/3, DirectWindow.Gfx->Width, DirectWindow.Gfx->Height, 1),
			vec4(CameraPos, 0),
			vec4(ViewDir, 0),
		};
		WorldUpdateBuffer.Update(DirectWindow.Gfx, (void*)&WorldUpdate);

		if(!DirectWindow.IsGfxPaused)
		{
			{
				FrustCullingContext.Begin(DirectWindow.Gfx->CmpCommandQueue, DirectWindow.Gfx->ResourceHeap.Handle.Get());

				FrustCullingContext.SetDescriptorTable(0, MeshOffsetsTable);
				FrustCullingContext.SetDescriptorTable(1, IndirectCommandsDataTable);
				FrustCullingContext.SetUnorderedAccessView(2, MeshCommonCullingData);

				FrustCullingContext.CommandList->CopyBufferRegion(IndirectDrawCommands.Handle.Get(), IndirectDrawCommands.CounterOffset, GeometriesCounterBuffer.Handle.Get(), 0, sizeof(u32));
				FrustCullingContext.CommandList->CopyBufferRegion(IndirectDrawIndexedCommands.Handle.Get(), IndirectDrawIndexedCommands.CounterOffset, GeometriesCounterBuffer.Handle.Get(), 0, sizeof(u32));

				D3D12_RESOURCE_BARRIER Barrier[] = 
				{
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawCommands.Handle.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawIndexedCommands.Handle.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
				};
				FrustCullingContext.CommandList->ResourceBarrier(2, Barrier);

				FrustCullingContext.Execute(DrawCount, 1, 1);

				FrustCullingContext.End(DirectWindow.Gfx->CmpCommandQueue);
			}

			{
				auto RtvHandle = RtvHeapMap[GfxContext.BackBufferIndex];
				auto DsvHandle = DsvHeapMap[0];

				GfxContext.Begin(DirectWindow.Gfx->GfxCommandQueue, DirectWindow.Gfx->ResourceHeap.Handle.Get());

				std::vector<CD3DX12_RESOURCE_BARRIER> Barrier = 
				{
					CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->BackBuffers[GfxContext.BackBufferIndex].Handle.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
					CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawIndexedCommands.Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
				};
				GfxContext.CommandList->ResourceBarrier(Barrier.size(), Barrier.data());

				GfxContext.SetTargetAndClear(&DirectWindow.Gfx->Viewport, &DirectWindow.Gfx->Rect, &RtvHandle, &DsvHandle);

				GfxContext.DrawIndirect(DrawCount, IndirectDrawIndexedCommands);
				
				Barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawIndexedCommands.Handle.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				Barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				GfxContext.CommandList->ResourceBarrier(2, Barrier.data());

				GfxContext.End(DirectWindow.Gfx->GfxCommandQueue);
			}

			{
				ReduceContext.Begin(DirectWindow.Gfx->CmpCommandQueue, DirectWindow.Gfx->ResourceHeap.Handle.Get());

				std::vector<D3D12_RESOURCE_BARRIER> DepthBufferBarrier;
				for(u32 DepthIdx = 0;
					DepthIdx < DepthMipLevel;
					++DepthIdx)
				{
					DepthBufferBarrier.push_back(CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[DepthIdx].Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
				}
				ReduceContext.CommandList->ResourceBarrier(DepthBufferBarrier.size(), DepthBufferBarrier.data());
				
				D3D12_GPU_DESCRIPTOR_HANDLE In  = {};
				D3D12_GPU_DESCRIPTOR_HANDLE Out = {};
				u32 NewHiWidth  = HiWidth;
				u32 NewHiHeight = HiHeight;
				for(u32 DepthTextureIdx = 0;
					DepthTextureIdx < DepthMipLevel;
					++DepthTextureIdx)
				{
					DepthTextureIdx == 0 ? 
						ReduceContext.SetDescriptorTable(0, DepthBufferTable) : 
						ReduceContext.SetDescriptorTable(0, HiZInputTable, DepthTextureIdx - 1);
					DepthTextureIdx == 0 ? 
						ReduceContext.SetDescriptorTable(1, HiZOutputTable) : 
						ReduceContext.SetDescriptorTable(1, HiZOutputTable, DepthTextureIdx);

					if(DepthTextureIdx != 0)
					{
						DepthBufferBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[DepthTextureIdx - 1].Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
						ReduceContext.CommandList->ResourceBarrier(1, DepthBufferBarrier.data());
					}

					NewHiWidth  = max(1, HiWidth  >> (DepthTextureIdx));
					NewHiHeight = max(1, HiHeight >> (DepthTextureIdx));

					ReduceContext.Execute(NewHiWidth, NewHiHeight, 1);
				}

				DepthBufferBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[DepthMipLevel - 1].Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				ReduceContext.CommandList->ResourceBarrier(1, DepthBufferBarrier.data());

				ReduceContext.End(DirectWindow.Gfx->CmpCommandQueue);
			}

			{
				OcclCullingContext.Begin(DirectWindow.Gfx->CmpCommandQueue, DirectWindow.Gfx->ResourceHeap.Handle.Get());

				OcclCullingContext.SetDescriptorTable(0, MeshOffsetsTable);
				OcclCullingContext.SetDescriptorTable(1, IndirectCommandsDataTable);
				OcclCullingContext.SetUnorderedAccessView(2, MeshCommonCullingData);
				OcclCullingContext.SetDescriptorTable(3, HiZInputTable);

				OcclCullingContext.Execute(DrawCount, 1, 1);

				OcclCullingContext.End(DirectWindow.Gfx->CmpCommandQueue);
			}

			{
				auto RtvHandle = RtvHeapMap[GfxContext.BackBufferIndex];
				auto DsvHandle = DsvHeapMap[0];

				GfxContext.Begin(DirectWindow.Gfx->GfxCommandQueue);

				std::vector<CD3DX12_RESOURCE_BARRIER> Barrier = 
				{
					CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawIndexedCommands.Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
					CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE),
				};
				GfxContext.CommandList->ResourceBarrier(Barrier.size(), Barrier.data());

				GfxContext.SetTarget(&DirectWindow.Gfx->Viewport, &DirectWindow.Gfx->Rect, &RtvHandle, &DsvHandle);

				GfxContext.DrawIndirect(DrawCount, IndirectDrawIndexedCommands);

				Barrier.clear();
				for(u32 DepthIdx = 0;
					DepthIdx < DepthMipLevel;
					++DepthIdx)
				{
					Barrier.push_back(CD3DX12_RESOURCE_BARRIER::Transition(DepthTextures[DepthIdx].Handle.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON));
				}
				Barrier.push_back(CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawCommands.Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON));
				Barrier.push_back(CD3DX12_RESOURCE_BARRIER::Transition(IndirectDrawIndexedCommands.Handle.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COMMON));
				
				Barrier.push_back(CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->BackBuffers[GfxContext.BackBufferIndex].Handle.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
				Barrier.push_back(CD3DX12_RESOURCE_BARRIER::Transition(DirectWindow.Gfx->DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
				GfxContext.CommandList->ResourceBarrier(Barrier.size(), Barrier.data());

				GfxContext.Present(DirectWindow.Gfx->GfxCommandQueue, DirectWindow.Gfx->SwapChain.Get());
			}
		}


		double TimeEnd = window::GetTimestamp();
		double TimeElapsed = (TimeEnd - TimeLast);

		vec3 Disp;
		if(DirectWindow.Buttons[EC_R].IsDown)
		{
			Disp = vec3(0,  0.1, 0);
			CameraPos += Disp;
		}
		if(DirectWindow.Buttons[EC_F].IsDown)
		{
			Disp = vec3(0, -0.1, 0);
			CameraPos += Disp;
		}
		if(DirectWindow.Buttons[EC_W].IsDown)
		{
			Disp = ViewDir * 0.1;
			CameraPos += Disp;
		}
		if(DirectWindow.Buttons[EC_S].IsDown)
		{
			Disp = ViewDir * 0.1;
			CameraPos -= Disp;
		}
		if(DirectWindow.Buttons[EC_A].IsDown || DirectWindow.Buttons[EC_LEFT].IsDown)
		{
			quat ViewDirQuat(ViewDir, 0);
			quat RotQuat(0.01, vec3(0, 1, 0));
			ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}
		if(DirectWindow.Buttons[EC_D].IsDown || DirectWindow.Buttons[EC_RIGHT].IsDown)
		{
			quat ViewDirQuat(ViewDir, 0);
			quat RotQuat(-0.01, vec3(0, 1, 0));
			ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}
		// NOTE: This works not as expected
		if(DirectWindow.Buttons[EC_UP].IsDown)
		{
			quat ViewDirQuat(ViewDir, 0);
			quat RotQuat(0.01, vec3(1, 0, 0));
			ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}
		if(DirectWindow.Buttons[EC_DOWN].IsDown)
		{
			quat ViewDirQuat(ViewDir, 0);
			quat RotQuat(-0.01, vec3(1, 0, 0));
			ViewDir = (RotQuat * ViewDirQuat * RotQuat.Inverse()).q;
		}

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

