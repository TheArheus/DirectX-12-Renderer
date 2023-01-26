#include "directx/d3dx12.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <D3DCompiler.h>
#include <DirectXMath.h>
#include <wrl.h>

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
	bool UseWarp = false;

	D3D12_VIEWPORT Viewport;
	D3D12_RECT Rect;

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
	void CommandBegin();
	void CommandsEnd();
public:
	d3d_app(UINT ClientWidth, UINT ClientHeight);

	void OnInit();
	void OnUpdate();
	void OnRender();
	void Flush();

	void WaitForGpu();
	void MoveToNextFrame();
};

class win_app
{
	d3d_app* App;
	static HWND Window;
	static bool IsRunning;

	static LRESULT CALLBACK WindowProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	int DispatchMessages();
public:
	win_app(d3d_app* App, HINSTANCE CurrInst, int CmdShow, UINT _Width, UINT _Height);
	int Run();
	static HWND GetWindowHandle() 
	{
		return Window;
	}
};

HWND win_app::Window = 0;
bool win_app::IsRunning = false;

win_app::
win_app(d3d_app* _App, HINSTANCE CurrInst, int CmdShow, UINT _Width, UINT _Height)
{
	App = _App;

	const char ClassName[] = "Direct 3D Engine";
	WNDCLASS WindowClass = {};
	WindowClass.lpfnWndProc = WindowProc;
	WindowClass.hInstance = CurrInst;
	WindowClass.lpszClassName = ClassName;
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClass(&WindowClass);

	Window = CreateWindowEx(0,
		ClassName, "Window", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		_Width, _Height,
		0, 0, CurrInst, _App);

	if (Window == NULL) return;

	RECT ClientRect = {};
	GetClientRect(Window, &ClientRect);
	int ClientWidth = ClientRect.right - ClientRect.left;
	int ClientHeight = ClientRect.bottom - ClientRect.top;

	IsRunning = true;
	App->OnInit();

	ShowWindow(Window, CmdShow);
	UpdateWindow(Window);
}

int win_app::Run() 
{
	int Result = 0;
	while (IsRunning)
	{
		Result = DispatchMessages();
	}

	return Result;
}

d3d_app::
d3d_app(UINT ClientWidth, UINT ClientHeight) : Width(ClientWidth), Height(ClientHeight)
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
};

void d3d_app::OnInit() 
{
	HRESULT HardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
	if (FAILED(HardwareResult))
	{
		ComPtr<IDXGIAdapter> WarpAdapter;
		Factory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter));
		D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
	}

	Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
	FenceEvent = CreateEvent(nullptr, 0, 0, nullptr);

	bool TearingSupport = false;
	if(SUCCEEDED(Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &TearingSupport, sizeof(TearingSupport))))
	{
		TearingSupport = true;
	}

	bool MsaaState = true;
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS MsQualityLevels = {};
	MsQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count = MsaaState ? 4 : 1;
		SwapChainDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
		Factory->CreateSwapChainForHwnd(CommandQueue.Get(), win_app::GetWindowHandle(), &SwapChainDesc, nullptr, nullptr, _SwapChain.GetAddressOf());
		_SwapChain.As(&SwapChain);
	}
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
		DepthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		DepthStencilDesc.SampleDesc.Count = MsaaState ? 4 : 1;
		DepthStencilDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
		DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE Clear = {};
		Clear.Format = DXGI_FORMAT_D32_FLOAT;
		Clear.DepthStencil.Depth = 1.0f;
		Clear.DepthStencil.Stencil = 0;
		Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&DepthStencilBuffer));

		Device->CreateDepthStencilView(DepthStencilBuffer.Get(), nullptr, DsvHeap->GetCPUDescriptorHandleForHeapStart());


		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
		WaitForGpu();
	}

	{
		vertex Vertices[] = 
		{
			{{ 0.0,  0.5, 0.0}, {1, 0, 0, 1}},
			{{ 0.5, -0.5, 0.0}, {0, 1, 0, 1}},
			{{-0.5, -0.5, 0.0}, {0, 0, 1, 1}},
		};
		const UINT VertexBufferSize = sizeof(Vertices);

		Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));

		Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize), D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&VertexBuffer));

		D3D12_SUBRESOURCE_DATA SubresData = {};
		SubresData.pData = Vertices;
		SubresData.RowPitch = VertexBufferSize;
		SubresData.SlicePitch = VertexBufferSize;

		
		CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(VertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		UpdateSubresources<1>(CommandList.Get(), VertexBuffer.Get(), TempBuffer.Get(), 0, 0, 1, &SubresData);
		CommandsEnd();
		WaitForGpu();

		VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
		VertexBufferView.StrideInBytes = sizeof(vertex);
		VertexBufferView.SizeInBytes = VertexBufferSize;
	}

	{
		{
			CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
			RootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
			PipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			PipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
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

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

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

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	CommandsEnd();

	SwapChain->Present(0, 0);

	//Flush();
	MoveToNextFrame();
};

void d3d_app::
Flush()
{
	CurrentFence++;
	CommandQueue->Signal(Fence.Get(), CurrentFence);
	UINT64 PrevFence = Fence->GetCompletedValue();
	if(PrevFence < CurrentFence)
	{
		HANDLE Event = CreateEventEx(nullptr, 0, 0, EVENT_ALL_ACCESS);
		Fence->SetEventOnCompletion(CurrentFence, Event);
		WaitForSingleObject(Event, INFINITE);
		CloseHandle(Event);
	}
}

void d3d_app::
WaitForGpu() 
{
	CommandQueue->Signal(Fence.Get(), CurrentFence);

	Fence->SetEventOnCompletion(CurrentFence, FenceEvent);
	WaitForSingleObject(FenceEvent, INFINITE);

	CurrentFence++;
}

void d3d_app::
MoveToNextFrame() 
{
	CommandQueue->Signal(Fence.Get(), CurrentFence);

	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	if (Fence->GetCompletedValue() < CurrentFence)
	{
		Fence->SetEventOnCompletion(CurrentFence, FenceEvent);
		WaitForSingleObject(FenceEvent, INFINITE);
	}

	CurrentFence++;
}

void d3d_app::
CommandsBegin(ID3D12PipelineState* _PipelineState)
{
	CommandAlloc->Reset();
	CommandList->Reset(CommandAlloc.Get(), _PipelineState);
}


void d3d_app::
CommandBegin()
{
	//CommandAlloc->Reset();
	//CommandList->Reset(CommandAlloc.Get(), _PipelineState);
}

void d3d_app::
CommandsEnd()
{
	CommandList->Close();
	ID3D12CommandList* CmdLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CmdLists), CmdLists);
}


int WinMain(HINSTANCE CurrInst, HINSTANCE PrevInst, PSTR Cmd, int Show)
{	
	d3d_app D3dApp(1240, 720);
	win_app WinApp(&D3dApp, CurrInst, Show, 1240, 720);
	return WinApp.Run();
}

int win_app::
DispatchMessages()
{
	MSG Message = {};

	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch(Message.message)
		{
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				unsigned int KeyCode = (unsigned int)Message.wParam;

				bool IsDown = ((Message.lParam & (1 << 31)) == 0);
				bool WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool IsLeftShift = KeyCode & VK_LSHIFT;

				if(IsDown)
				{
					if(KeyCode == 'R')
					{
					}
				}
			} break;
			default:
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}
		}
	}

	return static_cast<char>(Message.wParam);
}

LRESULT CALLBACK win_app::
WindowProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	d3d_app* App = reinterpret_cast<d3d_app*>(GetWindowLongPtr(Wnd, GWLP_USERDATA));
	switch (Msg)
	{
		case WM_CREATE:
		{
			// Save the DXSample* passed in to CreateWindow.
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(Wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		} break;
	
		case WM_QUIT:
		{
			DestroyWindow(Wnd);
			IsRunning = false;
		} break;

		case WM_DESTROY:
		{
			IsRunning = false;
			PostQuitMessage(0);
		} break;

		case WM_PAINT:
		{
			App->OnUpdate();
			App->OnRender();
		} break;

		default:
		{
			return DefWindowProc(Wnd, Msg, wParam, lParam);
		} break;
	}

	return 0;
}
