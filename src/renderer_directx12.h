#ifndef RENDERER_DIRECTX_12_H_

class command_queue
{
	D3D12_COMMAND_LIST_TYPE Type;
	ID3D12Device6* Device;
	std::vector<ID3D12GraphicsCommandList*> CommandLists;

public:
	ComPtr<ID3D12CommandQueue> Handle;
	ComPtr<ID3D12CommandAllocator> CommandAlloc;

	command_queue() = default;

	command_queue(ID3D12Device6* NewDevice, D3D12_COMMAND_LIST_TYPE NewType)
	{
		Init(NewDevice, NewType);
	}

	void Init(ID3D12Device6* NewDevice, D3D12_COMMAND_LIST_TYPE NewType)
	{
		Device = NewDevice;
		Type = NewType;

		D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
		CommandQueueDesc.Type = NewType;
		CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&Handle));

		Device->CreateCommandAllocator(Type, IID_PPV_ARGS(&CommandAlloc));
	}

	ID3D12GraphicsCommandList* AllocateCommandList()
	{
		ID3D12GraphicsCommandList* Result;
		Device->CreateCommandList(0, Type, CommandAlloc.Get(), nullptr, IID_PPV_ARGS(&Result));
		Result->Close();
		CommandLists.push_back(Result);

		return Result;
	}

	void Reset()
	{
		CommandAlloc->Reset();
	}

	void Execute()
	{
		for(ID3D12GraphicsCommandList* CommandList : CommandLists)
		{
			CommandList->Close();
		}

		std::vector<ID3D12CommandList*> CmdLists(CommandLists.begin(), CommandLists.end());
		Handle->ExecuteCommandLists(CmdLists.size(), CmdLists.data());
	}

	void Execute(ID3D12GraphicsCommandList* CommandList)
	{
		CommandList->Close();
		ID3D12CommandList* CmdLists[] = { CommandList };
		Handle->ExecuteCommandLists(_countof(CmdLists), CmdLists);
	}

	void ExecuteAndRemove(ID3D12GraphicsCommandList* CommandList)
	{
		Execute(CommandList);
		CommandLists.erase(std::remove(CommandLists.begin(), CommandLists.end(), CommandList), CommandLists.end());
	}
};

class fence
{
public:
	fence() = default;
	fence(ID3D12Device6* Device)
	{
		Init(Device);
	}

	void Init(ID3D12Device6* Device)
	{
		Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
		FenceEvent = CreateEvent(nullptr, 0, 0, nullptr);
	}

	void FenceSignal(command_queue& CommandQueue)
	{
		CommandQueue.Handle->Signal(Fence.Get(), ++CurrentFence);
	}

	void FenceWait()
	{
		if(Fence->GetCompletedValue() < CurrentFence)
		{
			HANDLE Event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
			Fence->SetEventOnCompletion(CurrentFence, Event);
			WaitForSingleObject(Event, INFINITE);
			CloseHandle(Event);
		}
	}

	void Flush(command_queue& CommandQueue)
	{
		FenceSignal(CommandQueue);
		FenceWait();
	}

private:
	u64 CurrentFence = 0;
	ComPtr<ID3D12Fence> Fence;
	HANDLE FenceEvent;
};

struct heap_alloc
{
private:
	u32 Next = 0;

	u32 AllocInc;
	u32 Range;

public:
	heap_alloc(D3D12_CPU_DESCRIPTOR_HANDLE NewCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE NewGpuHandle, u32 NewRange, u32 NewAllocInc) :
		CpuHandle(NewCpuHandle), GpuHandle(NewGpuHandle), Range(NewRange), AllocInc(NewAllocInc)
	{}

	D3D12_CPU_DESCRIPTOR_HANDLE GetNextCpuHandle()
	{
		D3D12_CPU_DESCRIPTOR_HANDLE Result = {CpuHandle.ptr + Next * AllocInc};
		assert(Next < Range);
		Next++;
		return Result;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE operator[](u32 Idx)
	{
		assert(Idx < Range);
		D3D12_CPU_DESCRIPTOR_HANDLE Result = {CpuHandle.ptr + Idx * AllocInc};
		return Result;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(u32 Idx)
	{
		assert(Idx < Range);
		D3D12_GPU_DESCRIPTOR_HANDLE Result = {GpuHandle.ptr + Idx * AllocInc};
		return Result;
	}

	template<typename T>
	void PushShaderResourceBufferView(std::unique_ptr<T>& App, const buffer& Buffer, size_t ElementSize, size_t ElementCount = 1)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Buffer.FirstElement = 0;
		SrvDesc.Buffer.NumElements = ElementCount;
		SrvDesc.Buffer.StructureByteStride = ElementSize;
		SrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		App->Device->CreateShaderResourceView(Buffer.Handle.Get(), &SrvDesc, GetNextCpuHandle());
	}

	template<typename T>
	void PushShaderResourceTexture2DView(std::unique_ptr<T>& App, const texture& Texture)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Texture2D.MipLevels = 1;
		SrvDesc.Texture2D.MostDetailedMip = 0;
		SrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		App->Device->CreateShaderResourceView(Texture.Handle.Get(), &SrvDesc, GetNextCpuHandle());
	}

	template<typename T>
	void PushUnorderedAccessBufferView(std::unique_ptr<T>& App, const buffer& Buffer, size_t ElementSize, size_t ElementCount = 1)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
		UavDesc.Format = DXGI_FORMAT_UNKNOWN;
		UavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		UavDesc.Buffer.FirstElement = 0;
		UavDesc.Buffer.NumElements = ElementCount;
		UavDesc.Buffer.StructureByteStride = ElementSize;
		UavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		App->Device->CreateUnorderedAccessView(Buffer.Handle.Get(), nullptr, &UavDesc, GetNextCpuHandle());
	}

	template<typename T>
	void PushUnorderedAccessBufferViewWithCounter(std::unique_ptr<T>& App, const buffer& CounterBuffer, const buffer& Buffer, size_t ElementSize, size_t ElementCount = 1)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
		UavDesc.Format = DXGI_FORMAT_UNKNOWN;
		UavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		UavDesc.Buffer.FirstElement = 0;
		UavDesc.Buffer.NumElements = ElementCount;
		UavDesc.Buffer.StructureByteStride = ElementSize;
		UavDesc.Buffer.CounterOffsetInBytes = CounterBuffer.CounterOffset;
		UavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		App->Device->CreateUnorderedAccessView(Buffer.Handle.Get(), CounterBuffer.Handle.Get(), &UavDesc, GetNextCpuHandle());
	}

	template<typename T>
	void PushUnorderedAccessTexture2DView(std::unique_ptr<T>& App, const texture& Texture, u32 MipSlice = 0)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
		UavDesc.Format = DXGI_FORMAT_R32_FLOAT;
		UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		UavDesc.Texture2D.PlaneSlice = 0;
		UavDesc.Texture2D.MipSlice = MipSlice;
		App->Device->CreateUnorderedAccessView(Texture.Handle.Get(), nullptr, &UavDesc, GetNextCpuHandle());
	}

	template<typename T>
	void PushConstantBufferView(std::unique_ptr<T>& App, const buffer& Buffer)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CbvDesc = {};
		CbvDesc.BufferLocation = Buffer.GpuPtr;
		CbvDesc.SizeInBytes = Buffer.Size;
		App->Device->CreateConstantBufferView(&CbvDesc, GetNextCpuHandle());
	}

	void Reset()
	{
		Next = 0;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;
};

class descriptor_heap
{
	u32 Size;
	u32 Total;

public:
	descriptor_heap() = default;

	descriptor_heap(ID3D12Device1* Device, u32 NumberOfDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
	{
		Init(Device, NumberOfDescriptors, Type, Flags);
	}

	void Init(ID3D12Device1* Device, u32 NumberOfDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.NumDescriptors = NumberOfDescriptors;
		HeapDesc.Type = Type;
		HeapDesc.Flags = Flags;
		Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Handle));

		AllocInc = Device->GetDescriptorHandleIncrementSize(Type);
		CpuBegin = CpuHandle = Handle->GetCPUDescriptorHandleForHeapStart();
		GpuBegin = GpuHandle = Handle->GetGPUDescriptorHandleForHeapStart();
	}

	void Reset()
	{
		CpuBegin = CpuHandle;
		GpuBegin = GpuHandle;
	}

	heap_alloc Allocate(u32 Range)
	{
		heap_alloc Result(CpuBegin, GpuBegin, Range, AllocInc);

		CpuBegin.ptr += AllocInc * Range;
		GpuBegin.ptr += AllocInc * Range;

		return Result;
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
	u32  MsaaQuality;
	b32  TearingSupport = false;
	bool MsaaState = false;
	bool UseWarp = false;

	const DXGI_FORMAT BackBufferFormat  = DXGI_FORMAT_R8G8B8A8_UNORM;
    const DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	ComPtr<IDXGIFactory6> Factory;

public:
	renderer_backend(HWND Window, u32 ClientWidth, u32 ClientHeight);

	void RecreateSwapchain(u32 NewWidth, u32 NewHeight);

	ID3D12PipelineState* CreateGraphicsPipeline(ID3D12RootSignature* GfxRootSignature, std::initializer_list<const std::string> ShaderList);
	ID3D12PipelineState* CreateComputePipeline(ID3D12RootSignature* RootSignature, const std::string& ComputeShader);

	void Present();

public:
	u32 Width;
	u32 Height;

	D3D12_VIEWPORT Viewport;
	D3D12_RECT Rect;

	// NOTE: It is temporary desicion. 
	// There will be its own allocator for descriptor heaps
	descriptor_heap RtvHeap;
	descriptor_heap DsvHeap;
	descriptor_heap ResourceHeap;
	texture BackBuffers[2];
	texture DepthStencilBuffer;
	int BackBufferIndex = 0;

	ComPtr<IDXGISwapChain4> SwapChain;
	ComPtr<ID3D12Device6> Device;

	command_queue GfxCommandQueue;
	command_queue CmpCommandQueue;

	fence Fence;
};

#define RENDERER_DIRECTX_12_H_
#endif
