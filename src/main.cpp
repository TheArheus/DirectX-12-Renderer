#include "directx/d3dx12.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <D3DCompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "win32_window.h"

#include <any>

using namespace Microsoft::WRL;
using namespace DirectX;

class d3d_app 
{
	UINT Width;
	UINT Height;

	struct vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT4 Col;
	};

	UINT MsaaQuality;
	bool MsaaState = true;
	bool TearingSupport = false;
	bool UseWarp = false;

	D3D12_VIEWPORT Viewport;
	D3D12_RECT Rect;

	const DXGI_FORMAT BackBufferFormat  = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	ComPtr<IDXGIFactory6> Factory;
	ComPtr<ID3D12Device6> Device;
	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12CommandAllocator> CommandAlloc;
	ComPtr<ID3D12GraphicsCommandList> CommandList;
	ComPtr<IDXGISwapChain4> SwapChain;
	ComPtr<ID3D12DescriptorHeap> RtvHeap;
	ComPtr<ID3D12DescriptorHeap> DsvHeap;
	ComPtr<ID3D12Resource> BackBuffers[2];
	ComPtr<ID3D12Resource> DepthStencilBuffer;
	UINT RtvSize;
	int BackBufferIndex = 0;

	ComPtr<ID3D12RootSignature> RootSignature;
	ComPtr<ID3D12PipelineState> PipelineState;

	UINT64 CurrentFence = 0;
	ComPtr<ID3D12Fence> Fence;
	HANDLE FenceEvent;

	ComPtr<ID3D12Resource> TempBuffer;
	ComPtr<ID3D12Resource> VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

	void CommandsBegin(ID3D12PipelineState* PipelineState = nullptr);
	void CommandsEnd();
public:
	d3d_app(HWND Window, UINT ClientWidth, UINT ClientHeight);

	void OnInit();
	void OnUpdate();
	void OnRender();

	void FenceSignal();
	void FenceWait();
	void Flush();
};

d3d_app::
d3d_app(HWND Window, UINT ClientWidth, UINT ClientHeight) : Width(ClientWidth), Height(ClientHeight)
{
	Viewport.Width = (FLOAT)Width;
	Viewport.Height = (FLOAT)Height;
	Viewport.MaxDepth = 1.0f;

	Rect = { 0, 0, (LONG)ClientWidth, (LONG)ClientHeight };

#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> Debug;
		D3D12GetDebugInterface(IID_PPV_ARGS(&Debug));
		Debug->EnableDebugLayer();
	}
#endif
	
	CreateDXGIFactory1(IID_PPV_ARGS(&Factory));

	HRESULT HardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
	if (FAILED(HardwareResult))
	{
		ComPtr<IDXGIAdapter> WarpAdapter;
		Factory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter));
		D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
	}

	Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
	FenceEvent = CreateEvent(nullptr, 0, 0, nullptr);

	if(SUCCEEDED(Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &TearingSupport, sizeof(TearingSupport))))
	{
		TearingSupport = true;
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS MsQualityLevels = {};
	MsQualityLevels.Format = BackBufferFormat;
	MsQualityLevels.SampleCount = 4;
	Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &MsQualityLevels, sizeof(MsQualityLevels));
	MsaaQuality = MsQualityLevels.NumQualityLevels;
	if (MsaaQuality < 2) MsaaState = false;

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&CommandQueue));

	Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAlloc));

	Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAlloc.Get(), nullptr, IID_PPV_ARGS(&CommandList));
	CommandList->Close();

	{
		ComPtr<IDXGISwapChain1> _SwapChain;
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Flags = TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
		SwapChainDesc.Width = Width;
		SwapChainDesc.Height = Height;
		SwapChainDesc.BufferCount = 2;
		SwapChainDesc.Format = BackBufferFormat;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count = MsaaState ? 4 : 1;
		SwapChainDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
		Factory->CreateSwapChainForHwnd(CommandQueue.Get(), Window, &SwapChainDesc, nullptr, nullptr, _SwapChain.GetAddressOf());
		_SwapChain.As(&SwapChain);
	}
};

void d3d_app::OnInit() 
{
	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	CommandsBegin();
	{
		D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
		RtvHeapDesc.NumDescriptors = 2;
		RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		Device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&RtvHeap));
		RtvSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc = {};
		DsvHeapDesc.NumDescriptors = 1;
		DsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		DsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		Device->CreateDescriptorHeap(&DsvHeapDesc, IID_PPV_ARGS(&DsvHeap));

		CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHeapHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT Idx = 0;
			Idx < 2;
			++Idx)
		{
			SwapChain->GetBuffer(Idx, IID_PPV_ARGS(&BackBuffers[Idx]));
			Device->CreateRenderTargetView(BackBuffers[Idx].Get(), nullptr, RtvHeapHandle);
			RtvHeapHandle.Offset(1, RtvSize);
		}

		D3D12_RESOURCE_DESC DepthStencilDesc = {};
		DepthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		DepthStencilDesc.Width = Width;
		DepthStencilDesc.Height = Height;
		DepthStencilDesc.DepthOrArraySize = 1;
		DepthStencilDesc.MipLevels = 1;
		DepthStencilDesc.Format = DepthBufferFormat;
		DepthStencilDesc.SampleDesc.Count = MsaaState ? 4 : 1;
		DepthStencilDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
		DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE Clear = {};
		Clear.Format = DepthBufferFormat;
		Clear.DepthStencil.Depth = 1.0f;
		Clear.DepthStencil.Stencil = 0;
		auto HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		Device->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&DepthStencilBuffer));

		Device->CreateDepthStencilView(DepthStencilBuffer.Get(), nullptr, DsvHeap->GetCPUDescriptorHandleForHeapStart());

		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		CommandList->ResourceBarrier(1, &Barrier);
		Flush();
	}

	{
		vertex Vertices[] = 
		{
			{{ 0.0,  0.5, 0.0}, {1, 0, 0, 1}},
			{{ 0.5, -0.5, 0.0}, {0, 1, 0, 1}},
			{{-0.5, -0.5, 0.0}, {0, 0, 1, 1}},
		};
		const UINT VertexBufferSize = sizeof(Vertices);

		auto TempBufferProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto TempBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
		Device->CreateCommittedResource(&TempBufferProp, D3D12_HEAP_FLAG_NONE, &TempBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));

		auto VertexBufferProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto VertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
		Device->CreateCommittedResource(&VertexBufferProp, D3D12_HEAP_FLAG_NONE, &VertexBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&VertexBuffer));

		D3D12_SUBRESOURCE_DATA SubresData = {};
		SubresData.pData = Vertices;
		SubresData.RowPitch = VertexBufferSize;
		SubresData.SlicePitch = VertexBufferSize;

		
		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(VertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CommandList->ResourceBarrier(1, &Barrier);
		UpdateSubresources<1>(CommandList.Get(), VertexBuffer.Get(), TempBuffer.Get(), 0, 0, 1, &SubresData);
		CommandsEnd();
		Flush();

		VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
		VertexBufferView.StrideInBytes = sizeof(vertex);
		VertexBufferView.SizeInBytes = VertexBufferSize;
	}

	{
		{
			CD3DX12_ROOT_PARAMETER RootParameter = {};
			RootParameter.InitAsUnorderedAccessView(0);
			CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
			RootSignatureDesc.Init(1, &RootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> Signature;
			ComPtr<ID3DBlob> Error;
			D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &Signature, &Error);
			Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
		}
		
		{
			ComPtr<ID3DBlob> VertShader;
			ComPtr<ID3DBlob> FragShader;
			D3DReadFileToBlob(L"..\\build\\mesh.vert.cso", &VertShader);
			D3DReadFileToBlob(L"..\\build\\mesh.frag.cso", &FragShader);

			D3D12_INPUT_ELEMENT_DESC InputDesc[] = 
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
				{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
			};

			D3D12_RASTERIZER_DESC RasterDesc = {};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineDesc = {};
			PipelineDesc.InputLayout = {InputDesc, _countof(InputDesc)};
			PipelineDesc.pRootSignature = RootSignature.Get();
			PipelineDesc.VS = CD3DX12_SHADER_BYTECODE(VertShader.Get());
			PipelineDesc.PS = CD3DX12_SHADER_BYTECODE(FragShader.Get());
			PipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			PipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			PipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			PipelineDesc.SampleMask = UINT_MAX;
			PipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			PipelineDesc.NumRenderTargets = 1;
			PipelineDesc.SampleDesc.Count = MsaaState ? 4 : 1;
			PipelineDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
			PipelineDesc.RTVFormats[0] = BackBufferFormat;
			PipelineDesc.DSVFormat = DepthBufferFormat;
			PipelineDesc.SampleDesc.Count = 1;
			Device->CreateGraphicsPipelineState(&PipelineDesc, IID_PPV_ARGS(&PipelineState));
		}
	}
}

void d3d_app::OnUpdate()
{
};

void d3d_app::OnRender()
{
	CommandsBegin(PipelineState.Get());

	CommandList->SetGraphicsRootSignature(RootSignature.Get());
	CommandList->SetPipelineState(PipelineState.Get());

	auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &Barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE RenderViewHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart(), BackBufferIndex, RtvSize);
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView = DsvHeap->GetCPUDescriptorHandleForHeapStart();
	CommandList->OMSetRenderTargets(1, &RenderViewHandle, false, &DepthStencilView);
	CommandList->RSSetViewports(1, &Viewport);
	CommandList->RSSetScissorRects(1, &Rect);

	float ClearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	CommandList->ClearRenderTargetView(RenderViewHandle, ClearColor, 0, nullptr);
	CommandList->ClearDepthStencilView(DsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0F, 0, 0, nullptr);
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
	CommandList->DrawInstanced(3, 1, 0, 0);

	Barrier = CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &Barrier);

	CommandsEnd();

	SwapChain->Present(0, 0);

	Flush();
	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
};

void d3d_app::
Flush()
{
	FenceSignal();
	FenceWait();
}

void d3d_app::
FenceSignal()
{
	CommandQueue->Signal(Fence.Get(), ++CurrentFence);
}

void d3d_app::
FenceWait()
{
	if(Fence->GetCompletedValue() < CurrentFence)
	{
		HANDLE Event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		Fence->SetEventOnCompletion(CurrentFence, Event);
		WaitForSingleObject(Event, INFINITE);
		CloseHandle(Event);
	}
}

void d3d_app::
CommandsBegin(ID3D12PipelineState* _PipelineState)
{
	CommandAlloc->Reset();
	CommandList->Reset(CommandAlloc.Get(), _PipelineState);
}

void d3d_app::
CommandsEnd()
{
	CommandList->Close();
	ID3D12CommandList* CmdLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CmdLists), CmdLists);
}

class app
{
		
};

int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{	
	window DirectWindow(1240, 720, "3D Renderer");
	d3d_app Graphics(DirectWindow.Handle, DirectWindow.Width, DirectWindow.Height);
	Graphics.OnInit();

	while(DirectWindow.IsRunning())
	{
		auto Result = DirectWindow.ProcessMessages();
		if(Result) return *Result;

		Graphics.OnUpdate();
		Graphics.OnRender();
	}

	return 0;
}

int main(int argc, char* argv[])
{
	return WinMain(0, 0, argv[0], SW_SHOWNORMAL);
}

