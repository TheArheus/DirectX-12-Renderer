#ifndef RENDERER_DIRECTX_12_H_

using namespace Microsoft::WRL;
using namespace DirectX;

void GetDevice(ID3D12Device6** DeviceResult, bool HighPerformance = true)
{
	ComPtr<IDXGIFactory6> Factory;
	*DeviceResult = nullptr;
	CreateDXGIFactory1(IID_PPV_ARGS(&Factory));

	UINT i = 0;
	ComPtr<IDXGIAdapter1> pAdapter;
	for (u32 AdapterIndex = 0;
			Factory->EnumAdapterByGpuPreference(AdapterIndex, 
				(HighPerformance ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED), 
				 IID_PPV_ARGS(&pAdapter)) != DXGI_ERROR_NOT_FOUND;
		++AdapterIndex)
	{
		DXGI_ADAPTER_DESC1 Desc;
		pAdapter->GetDesc1(&Desc);

		if(SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device6), nullptr)))
		{
			break;
		}
	}

	D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(DeviceResult));
}

struct buffer
{
	template<class T>
	buffer(std::unique_ptr<T>& App, ID3D12Heap1* Heap, void* Data, u64 NewSize, u64 Offset) : Size(NewSize)
	{
		ComPtr<ID3D12Device6> Device;
		GetDevice(&Device);

		CD3DX12_HEAP_PROPERTIES ResourceTypeMain = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
		ComPtr<ID3D12Resource> TempBuffer;

		Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));
		Device->CreatePlacedResource(Heap, Offset, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));

		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempBuffer->Unmap(0, 0);

		App->CommandsBegin();
		App->CommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->CommandsEnd();
		App->Flush();

		GpuPtr = Handle->GetGPUVirtualAddress();
	}

	D3D12_GPU_VIRTUAL_ADDRESS GpuPtr = 0;
	ComPtr<ID3D12Resource> Handle;
	u64 Size;
};

struct vertex_buffer_view
{
	D3D12_VERTEX_BUFFER_VIEW Handle;
	u32 VertexBegin;
	u32 VertexCount;
};

struct index_buffer_view
{
	D3D12_INDEX_BUFFER_VIEW Handle;
	u32 IndexBegin;
	u32 IndexCount;
};

struct shader_resource_view
{
	D3D12_SHADER_RESOURCE_VIEW_DESC Handle;
};

struct unordered_access_view
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC Handle;
};

struct sampler
{
	D3D12_SAMPLER_DESC Handle;
};

class d3d_app;
class memory_heap
{
public:
	memory_heap(u64 NewSize) : Size(NewSize), Alignment(D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
	{
		ComPtr<ID3D12Device6> Device;
		GetDevice(&Device);

		D3D12_HEAP_PROPERTIES HeapProps = {};
	    HeapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;

		D3D12_HEAP_DESC HeapDesc = {};
		HeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
		HeapDesc.Properties = HeapProps;
		HeapDesc.SizeInBytes = Size;
		HeapDesc.Alignment = Alignment;

		Device->CreateHeap(&HeapDesc, IID_PPV_ARGS(&Handle));
	}

	void Reset()
	{
		BeginData = 0;
	}

	buffer PushBuffer(std::unique_ptr<d3d_app>& App, void* Data, u64 DataSize)
	{
		u64 AlignedSize = AlignUp(DataSize, Alignment);
		assert(AlignedSize + TotalSize <= Size);
		buffer Buffer(App, Handle.Get(), Data, DataSize, BeginData);
		BeginData += AlignedSize;
		TotalSize += AlignedSize;
		return Buffer;
	}

private:
	u64 Size = 0;
	u64 TotalSize = 0;
	u64 BeginData = 0;
	u64 Alignment = 0;

	ComPtr<ID3D12Heap1> Handle;
};

class d3d_app 
{
	u32 MsaaQuality;
	bool MsaaState = true;
	bool TearingSupport = false;
	bool UseWarp = false;

	D3D12_VIEWPORT Viewport;
	D3D12_RECT Rect;

	const DXGI_FORMAT BackBufferFormat  = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	ComPtr<IDXGIFactory6> Factory;
	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12CommandAllocator> CommandAlloc;
	ComPtr<IDXGISwapChain4> SwapChain;
	ComPtr<ID3D12DescriptorHeap> RtvHeap;
	ComPtr<ID3D12DescriptorHeap> DsvHeap;
	ComPtr<ID3D12Resource> BackBuffers[2];
	ComPtr<ID3D12Resource> DepthStencilBuffer;
	u32 RtvSize;
	int BackBufferIndex = 0;

	ComPtr<ID3D12RootSignature> RootSignature;
	ComPtr<ID3D12PipelineState> PipelineState;

	u64 CurrentFence = 0;
	ComPtr<ID3D12Fence> Fence;
	HANDLE FenceEvent;

	std::vector<vertex_buffer_view> VertexViews;
	std::vector<std::pair<vertex_buffer_view, index_buffer_view>> IndexedVertexViews;

public:
	d3d_app(HWND Window, u32 ClientWidth, u32 ClientHeight);

	void RecreateSwapchain(u32 NewWidth, u32 NewHeight);

	void PushVertexData(buffer* VertexBuffer, std::vector<mesh::offset>& Offsets);
	void PushIndexedVertexData(buffer* VertexBuffer, buffer* IndexBuffer, std::vector<mesh::offset>& Offsets);

	void CommandsBegin(ID3D12PipelineState* PipelineState = nullptr);
	void CommandsEnd();

	void OnInit();
	void BeginRender();
	void Draw();
	void DrawIndexed();
	void EndRender();

	void FenceSignal();
	void FenceWait();
	void Flush();

	u32 Width;
	u32 Height;

	double RefreshRate;

	ComPtr<ID3D12Device6> Device;
	ComPtr<ID3D12GraphicsCommandList> CommandList;
};

#define RENDERER_DIRECTX_12_H_
#endif
