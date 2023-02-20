#ifndef RENDERER_DIRECTX_12_H_

class descriptor_heap
{
	u32 Size;
	u32 Total;

public:
	enum type : u32
	{
		_shader_resource_view	= 0x1,
		_unordered_access_view	= 0x2,
		_constant_buffer_view	= 0x4,
		_sampler				= 0x8,
	};

	descriptor_heap(ID3D12Device1* Device, u32 NumberOfDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.NumDescriptors = NumberOfDescriptors;
		HeapDesc.Type = Type;
		HeapDesc.Flags = Flags;
		Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Handle));

		CpuBegin = CpuHandle = Handle->GetCPUDescriptorHandleForHeapStart();
		GpuBegin = GpuHandle = Handle->GetGPUDescriptorHandleForHeapStart();
	}

	void Reset()
	{
		CpuBegin = CpuHandle;
		GpuBegin = GpuHandle;
	}

	u32 AllocInc;
	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE CpuBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE GpuBegin;

	ComPtr<ID3D12DescriptorHeap> Handle;
};

class renderer_backend 
{
	u32 MsaaQuality;
	bool MsaaState = true;
	bool TearingSupport = false;
	bool UseWarp = false;

	const DXGI_FORMAT BackBufferFormat  = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	ComPtr<IDXGIFactory6> Factory;

	u64 CurrentFence = 0;
	ComPtr<ID3D12Fence> Fence;
	HANDLE FenceEvent;

	//std::vector<vertex_buffer_view> VertexViews;
	//std::vector<std::pair<vertex_buffer_view, index_buffer_view>> IndexedVertexViews;

public:
	renderer_backend(HWND Window, u32 ClientWidth, u32 ClientHeight);

	void RecreateSwapchain(u32 NewWidth, u32 NewHeight);

	//void PushVertexData(buffer* VertexBuffer, std::vector<mesh::offset>& Offsets);
	//void PushIndexedVertexData(buffer* VertexBuffer, buffer* IndexBuffer, std::vector<mesh::offset>& Offsets);

	ID3D12PipelineState* CreateGraphicsPipeline(ID3D12RootSignature* GfxRootSignature, std::initializer_list<const std::string> ShaderList);
	ID3D12PipelineState* CreateComputePipeline(ID3D12RootSignature* RootSignature, const std::string& ComputeShader);

	void FenceSignal(ID3D12CommandQueue* CommandQueue);
	void FenceWait();
	void Flush(ID3D12CommandQueue* CommandQueue);

	D3D12_VIEWPORT Viewport;
	D3D12_RECT Rect;

	u32 RtvSize;
	ComPtr<ID3D12DescriptorHeap> RtvHeap;
	ComPtr<ID3D12DescriptorHeap> DsvHeap;
	texture BackBuffers[2];
	texture DepthStencilBuffer;
	int BackBufferIndex = 0;

	ComPtr<IDXGISwapChain4> SwapChain;
	ComPtr<ID3D12Device6> Device;

	ComPtr<ID3D12CommandAllocator> GfxCommandAlloc;
	ComPtr<ID3D12CommandAllocator> CmpCommandAlloc;
	ComPtr<ID3D12CommandQueue> GfxCommandQueue;
	ComPtr<ID3D12CommandQueue> CmpCommandQueue;
	ComPtr<ID3D12GraphicsCommandList> GfxCommandList;
	ComPtr<ID3D12GraphicsCommandList> CmpCommandList;

	// NOTE: It is temporary desicion. 
	// There will be its own allocator for descriptor heaps
	ComPtr<ID3D12DescriptorHeap> GfxResourceHeap;
	ComPtr<ID3D12DescriptorHeap> CmpResourceHeap;
	u32 ResourceHeapIncrement;
};

#define RENDERER_DIRECTX_12_H_
#endif
