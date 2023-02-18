
#pragma once

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

struct buffer
{
	template<class T>
	buffer(std::unique_ptr<T>& App, ID3D12Heap1* Heap, 
		   u64 Offset, void* Data, u64 NewSize, u64 Alignment = 0, 
		   D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) : Size(NewSize)
	{
		ComPtr<ID3D12Resource> TempBuffer;
		ComPtr<ID3D12Device6> Device;

		u64 BufferSize = Alignment == 0 ? Size : AlignUp(Size, Alignment);
		Size = BufferSize;
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize, Flags);
		CD3DX12_RESOURCE_DESC TemporarDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);

		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &TemporarDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));
		App->Device->CreatePlacedResource(Heap, Offset, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));

		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempBuffer->Unmap(0, 0);

		App->GfxCommandAlloc->Reset();
		App->GfxCommandList->Reset(App->GfxCommandAlloc.Get(), nullptr);

		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandList->Close();
		ID3D12CommandList* GfxCmdLists[] = { App->GfxCommandList.Get() };
		App->GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);
		App->Flush(App->GfxCommandQueue.Get());

		GpuPtr = Handle->GetGPUVirtualAddress();
	}

	template<class T>
	buffer(std::unique_ptr<T>& App, ID3D12Heap1* Heap, 
		   u64 Offset, u64 NewSize, u64 Alignment = 0, 
		   D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) : Size(NewSize)
	{
		ComPtr<ID3D12Resource> TempBuffer;
		ComPtr<ID3D12Device6> Device;

		u64 BufferSize = Alignment == 0 ? Size : AlignUp(Size, Alignment);
		Size = BufferSize;
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize, Flags);
		CD3DX12_RESOURCE_DESC TemporarDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);

		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &TemporarDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));
		App->Device->CreatePlacedResource(Heap, Offset, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));

		GpuPtr = Handle->GetGPUVirtualAddress();
	}

	template<class T>
	buffer(std::unique_ptr<T>& App, 
		   void* Data, u64 NewSize, u64 Alignment = 0, 
		   D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) : Size(NewSize)
	{
		ComPtr<ID3D12Resource> TempBuffer;
		ComPtr<ID3D12Device6> Device;

		u64 BufferSize = Alignment == 0 ? Size : AlignUp(Size, Alignment);
		Size = BufferSize;
		CD3DX12_HEAP_PROPERTIES ResourceTypeMain = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize, Flags);
		CD3DX12_RESOURCE_DESC TemporarDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);

		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &TemporarDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));
		App->Device->CreateCommittedResource(&ResourceTypeMain, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));

		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempBuffer->Unmap(0, 0);

		App->GfxCommandAlloc->Reset();
		App->GfxCommandList->Reset(App->GfxCommandAlloc.Get(), nullptr);
		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandList->Close();
		ID3D12CommandList* GfxCmdLists[] = { App->GfxCommandList.Get() };
		App->GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);
		App->Flush(App->GfxCommandQueue.Get());

		GpuPtr = Handle->GetGPUVirtualAddress();
	}

	template<class T>
	buffer(std::unique_ptr<T>& App, 
		   u64 NewSize, u64 Alignment = 0, 
		   D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) : Size(NewSize)
	{
		ComPtr<ID3D12Resource> TempBuffer;
		ComPtr<ID3D12Device6> Device;

		u64 BufferSize = Alignment == 0 ? Size : AlignUp(Size, Alignment);
		Size = BufferSize;
		CD3DX12_HEAP_PROPERTIES ResourceTypeMain = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize, Flags);
		CD3DX12_RESOURCE_DESC TemporarDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);

		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &TemporarDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));
		App->Device->CreateCommittedResource(&ResourceTypeMain, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));

		GpuPtr = Handle->GetGPUVirtualAddress();
	}

	template<class T>
	void Update(std::unique_ptr<T>& App, void* Data)
	{
		ComPtr<ID3D12Resource> TempBuffer;
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC TemporarDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &TemporarDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));

		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempBuffer->Unmap(0, 0);

		App->GfxCommandAlloc->Reset();
		App->GfxCommandList->Reset(App->GfxCommandAlloc.Get(), nullptr);
		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandList->Close();
		ID3D12CommandList* GfxCmdLists[] = { App->GfxCommandList.Get() };
		App->GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);
		App->Flush(App->GfxCommandQueue.Get());
	}

	template<class T>
	void ReadBack(std::unique_ptr<T>& App, void* Data)
	{
		App->Flush(App->GfxCommandQueue.Get());

		ComPtr<ID3D12Resource> TempBuffer;
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		CD3DX12_RESOURCE_DESC TemporarDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &TemporarDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&TempBuffer));

		App->GfxCommandAlloc->Reset();
		App->GfxCommandList->Reset(App->GfxCommandAlloc.Get(), nullptr);

		CD3DX12_RESOURCE_BARRIER Barrier[] = 
		{
			CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE),
		};
		App->GfxCommandList->ResourceBarrier(1, Barrier);

		App->GfxCommandList->CopyResource(TempBuffer.Get(), Handle.Get());

		Barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		App->GfxCommandList->ResourceBarrier(1, Barrier);

		App->GfxCommandList->Close();
		ID3D12CommandList* GfxCmdLists[] = { App->GfxCommandList.Get() };
		App->GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);
		App->Flush(App->GfxCommandQueue.Get());

		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(Data, CpuPtr, Size);
		TempBuffer->Unmap(0, 0);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GpuPtr = 0;
	ComPtr<ID3D12Resource> Handle;
	u64 Size;
};

struct texture
{
	texture() = default;

	template<class T>
	texture(std::unique_ptr<T>& App, ID3D12Heap1* Heap, u64 Offset, void* Data, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, DXGI_FORMAT _Format = DXGI_FORMAT_R8G8B8A8_UINT, u32 MipLevels = 1, u64 Alignment = 0): Width(NewWidth), Height(NewHeight), Format(_Format)
	{
		ComPtr<ID3D12Resource> TempBuffer;
		ComPtr<ID3D12Device6> Device;

		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Format = Format;
		ResourceDesc.Alignment = Alignment;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Width = Width;
		ResourceDesc.Height = Height;
		ResourceDesc.DepthOrArraySize = DepthOrArraySize;
		ResourceDesc.MipLevels = MipLevels;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));
		App->Device->CreatePlacedResource(Heap, Offset, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));

		u64 Size = Width * Height * GetFormatSize(Format);
		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempBuffer->Unmap(0, 0);

		App->GfxCommandAlloc->Reset();
		App->GfxCommandList->Reset(App->GfxCommandAlloc.Get(), nullptr);
		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandList->Close();
		ID3D12CommandList* GfxCmdLists[] = { App->GfxCommandList.Get() };
		App->GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);
		App->Flush(App->GfxCommandQueue.Get());
	}

	template<class T>
	texture(std::unique_ptr<T>& App, ID3D12Heap1* Heap, u64 Offset, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, DXGI_FORMAT _Format = DXGI_FORMAT_R8G8B8A8_UINT, u32 MipLevels = 1, u64 Alignment = 0): Width(NewWidth), Height(NewHeight), Format(_Format)
	{
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Format = Format;
		ResourceDesc.Alignment = Alignment;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Width = Width;
		ResourceDesc.Height = Height;
		ResourceDesc.DepthOrArraySize = DepthOrArraySize;
		ResourceDesc.MipLevels = MipLevels;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = App->Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		App->Device->CreatePlacedResource(Heap, Offset, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));
	}

	template<class T>
	texture(std::unique_ptr<T>& App, void* Data, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, DXGI_FORMAT _Format = DXGI_FORMAT_R8G8B8A8_UINT, u32 MipLevels = 1, u64 Alignment = 0, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE): Width(NewWidth), Height(NewHeight), Format(_Format)
	{
		ComPtr<ID3D12Resource> TempBuffer;

		CD3DX12_HEAP_PROPERTIES ResourceTypeMain = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Format = Format;
		ResourceDesc.Alignment = Alignment;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Width = Width;
		ResourceDesc.Height = Height;
		ResourceDesc.DepthOrArraySize = DepthOrArraySize;
		ResourceDesc.MipLevels = MipLevels;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.Flags = Flags;

		D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = App->Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));
		App->Device->CreateCommittedResource(&ResourceTypeMain, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));

		u64 Size = Width * Height * GetFormatSize(Format);
		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempBuffer->Unmap(0, 0);

		App->GfxCommandAlloc->Reset();
		App->GfxCommandList->Reset(App->GfxCommandAlloc.Get(), nullptr);
		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandList->Close();
		ID3D12CommandList* GfxCmdLists[] = { App->GfxCommandList.Get() };
		App->GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);
		App->Flush(App->GfxCommandQueue.Get());
	}

	template<class T>
	texture(std::unique_ptr<T>& App, u64 NewWidth, u64 NewHeight, u64 DepthOrArraySize = 1, DXGI_FORMAT _Format = DXGI_FORMAT_R8G8B8A8_UINT, u32 MipLevels = 1, u64 Alignment = 0, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE): Width(NewWidth), Height(NewHeight), Format(_Format)
	{
		CD3DX12_HEAP_PROPERTIES ResourceTypeMain = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Format = Format;
		ResourceDesc.Alignment = Alignment;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Width = Width;
		ResourceDesc.Height = Height;
		ResourceDesc.DepthOrArraySize = DepthOrArraySize;
		ResourceDesc.MipLevels = MipLevels;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.Flags = Flags;

		D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = App->Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		App->Device->CreateCommittedResource(&ResourceTypeMain, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));
	}

	template<class T>
	void ReadBack(std::unique_ptr<T>& App, void* Data)
	{
		App->Flush(App->GfxCommandQueue.Get());

		ComPtr<ID3D12Resource> TempBuffer;
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Format = Format;
		ResourceDesc.Alignment = 0;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Width = Width;
		ResourceDesc.Height = Height;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.MipLevels = 1;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&TempBuffer));

		App->GfxCommandAlloc->Reset();
		App->GfxCommandList->Reset(App->GfxCommandAlloc.Get(), nullptr);

		CD3DX12_RESOURCE_BARRIER Barrier[] = 
		{
			CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE),
		};
		App->GfxCommandList->ResourceBarrier(1, Barrier);

		App->GfxCommandList->CopyResource(TempBuffer.Get(), Handle.Get());

		Barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		App->GfxCommandList->ResourceBarrier(1, Barrier);

		App->GfxCommandList->Close();
		ID3D12CommandList* GfxCmdLists[] = { App->GfxCommandList.Get() };
		App->GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);
		App->Flush(App->GfxCommandQueue.Get());

		u64 Size = Width * Height * GetFormatSize(Format);
		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(Data, CpuPtr, Size);
		TempBuffer->Unmap(0, 0);
	}

	ComPtr<ID3D12Resource> Handle;
	u64 Width;
	u64 Height;
	DXGI_FORMAT Format;
};

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

	template<class T>
	buffer PushBuffer(std::unique_ptr<T>& App, 
					  void* Data, u64 DataSize, 
					  D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
	{
		u64 AlignedSize = AlignUp(DataSize, Alignment);
		assert(AlignedSize + TotalSize <= Size);
		buffer Buffer(App, Handle.Get(), BeginData, Data, DataSize, 0, Flags);
		BeginData += AlignedSize;
		TotalSize += AlignedSize;
		return Buffer;
	}

	template<class T>
	buffer PushConstantBuffer(std::unique_ptr<T>& App, 
							  void* Data, u64 DataSize, 
							  D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
	{
		u64 AlignedSize = AlignUp(DataSize, 256);
		assert(AlignedSize + TotalSize <= Size);
		buffer Buffer(App, Handle.Get(), BeginData, Data, DataSize, 256, Flags);
		BeginData += AlignedSize;
		TotalSize += AlignedSize;
		return Buffer;
	}

	template<class T>
	texture PushTexture2D(std::unique_ptr<T>& App, 
						  void* Data, u64 Width, u64 Height, 
						  DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UINT, 
						  u32 MipLevels = 1, 
						  D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
	{
		u64 TextureAlignment = Alignment;
		u64 AlignedSize = AlignUp(Width * Height * GetFormatSize(Format), TextureAlignment);
		assert(AlignedSize + TotalSize <= Size);
		texture Texture(App, Handle.Get(), BeginData, Data, Width, Height, Format, MipLevels, Flags);
		BeginData += AlignedSize;
		TotalSize += AlignedSize;
		return Texture;
	}

private:
	u64 Size = 0;
	u64 TotalSize = 0;
	u64 BeginData = 0;
	u64 Alignment = 0;

	ComPtr<ID3D12Heap1> Handle;
};

