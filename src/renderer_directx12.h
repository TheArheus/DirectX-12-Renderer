#ifndef RENDERER_DIRECTX_12_H_

using namespace Microsoft::WRL;
using namespace DirectX;

#include "directx_utilities.hpp"

struct buffer
{
	template<class T>
	buffer(std::unique_ptr<T>& App, ID3D12Heap1* Heap, u64 Offset, void* Data, u64 NewSize, u64 Alignment = 0, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) : Size(NewSize)
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

		App->GfxCommandsBegin();
		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandsEnd();
		App->GfxFlush();

		GpuPtr = Handle->GetGPUVirtualAddress();
	}

	template<class T>
	buffer(std::unique_ptr<T>& App, ID3D12Heap1* Heap, u64 Offset, u64 NewSize, u64 Alignment = 0, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) : Size(NewSize)
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
	buffer(std::unique_ptr<T>& App, void* Data, u64 NewSize, u64 Alignment = 0, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) : Size(NewSize)
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

		App->GfxCommandsBegin();
		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandsEnd();
		App->GfxFlush();

		GpuPtr = Handle->GetGPUVirtualAddress();
	}

	template<class T>
	buffer(std::unique_ptr<T>& App, u64 NewSize, u64 Alignment = 0, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE) : Size(NewSize)
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

		App->GfxCommandsBegin();
		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandsEnd();
		App->GfxFlush();
	}

	template<class T>
	void ReadBack(std::unique_ptr<T>& App, void* Data)
	{
		App->GfxFlush();

		ComPtr<ID3D12Resource> TempBuffer;
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		CD3DX12_RESOURCE_DESC TemporarDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
		App->Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &TemporarDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&TempBuffer));

		App->GfxCommandsBegin();

		CD3DX12_RESOURCE_BARRIER Barrier[] = 
		{
			CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE),
		};
		App->GfxCommandList->ResourceBarrier(1, Barrier);

		App->GfxCommandList->CopyResource(TempBuffer.Get(), Handle.Get());

		Barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(Handle.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
		App->GfxCommandList->ResourceBarrier(1, Barrier);

		App->GfxCommandsEnd();
		App->GfxFlush();

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
	texture(std::unique_ptr<T>& App, ID3D12Heap1* Heap, u64 Offset, void* Data, u64 NewWidth, u64 NewHeight, DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UINT, u32 MipLevels = 1, u64 Alignment = 0): Width(NewWidth), Height(NewHeight)
	{
		ComPtr<ID3D12Resource> TempBuffer;
		ComPtr<ID3D12Device6> Device;
		GetDevice(&Device);

		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Format = Format;
		ResourceDesc.Alignment = Alignment;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Width = Width;
		ResourceDesc.Height = Height;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.MipLevels = MipLevels;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));
		Device->CreatePlacedResource(Heap, Offset, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));

		u64 Size = Width * Height * GetFormatSize(Format);
		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempBuffer->Unmap(0, 0);

		App->GfxCommandsBegin();
		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandsEnd();
		App->GfxFlush();

		GpuPtr = Handle->GetGPUVirtualAddress();
	}

	template<class T>
	texture(std::unique_ptr<T>& App, void* Data, u64 NewWidth, u64 NewHeight, DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UINT, u32 MipLevels = 1, u64 Alignment = 0, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE): Width(NewWidth), Height(NewHeight)
	{
		ComPtr<ID3D12Resource> TempBuffer;
		ComPtr<ID3D12Device6> Device;
		GetDevice(&Device);

		CD3DX12_HEAP_PROPERTIES ResourceTypeMain = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES ResourceTypeTemp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Format = Format;
		ResourceDesc.Alignment = Alignment;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Width = Width;
		ResourceDesc.Height = Height;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.MipLevels = MipLevels;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.Flags = Flags;

		D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		Device->CreateCommittedResource(&ResourceTypeTemp, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer));
		Device->CreateCommittedResource(&ResourceTypeMain, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Handle));

		u64 Size = Width * Height * GetFormatSize(Format);
		void* CpuPtr;
		TempBuffer->Map(0, nullptr, &CpuPtr);
		memcpy(CpuPtr, Data, Size);
		TempBuffer->Unmap(0, 0);

		App->GfxCommandsBegin();
		App->GfxCommandList->CopyResource(Handle.Get(), TempBuffer.Get());
		App->GfxCommandsEnd();
		App->GfxFlush();

		GpuPtr = Handle->GetGPUVirtualAddress();
	}

	D3D12_GPU_VIRTUAL_ADDRESS GpuPtr = 0;
	ComPtr<ID3D12Resource> Handle;
	u64 Width;
	u64 Height;
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

class resource_alloc
{
	ComPtr<ID3D12DescriptorHeap> Handle;
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

	buffer PushBuffer(std::unique_ptr<d3d_app>& App, void* Data, u64 DataSize, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
	{
		u64 AlignedSize = AlignUp(DataSize, Alignment);
		assert(AlignedSize + TotalSize <= Size);
		buffer Buffer(App, Handle.Get(), BeginData, Data, DataSize, 0, Flags);
		BeginData += AlignedSize;
		TotalSize += AlignedSize;
		return Buffer;
	}

	buffer PushConstantBuffer(std::unique_ptr<d3d_app>& App, void* Data, u64 DataSize, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
	{
		u64 AlignedSize = AlignUp(DataSize, 256);
		assert(AlignedSize + TotalSize <= Size);
		buffer Buffer(App, Handle.Get(), BeginData, Data, DataSize, 256, Flags);
		BeginData += AlignedSize;
		TotalSize += AlignedSize;
		return Buffer;
	}

	texture PushTexture2D(std::unique_ptr<d3d_app>& App, void* Data, u64 Width, u64 Height, DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UINT, u32 MipLevels = 1, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
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
	ComPtr<ID3D12CommandQueue> GfxCommandQueue;
	ComPtr<ID3D12CommandQueue> CmpCommandQueue;
	ComPtr<ID3D12CommandAllocator> GfxCommandAlloc;
	ComPtr<ID3D12CommandAllocator> CmpCommandAlloc;
	ComPtr<IDXGISwapChain4> SwapChain;
	ComPtr<ID3D12DescriptorHeap> RtvHeap;
	ComPtr<ID3D12DescriptorHeap> DsvHeap;
	texture BackBuffers[2];
	texture DepthStencilBuffer;
	u32 RtvSize;
	int BackBufferIndex = 0;

	ComPtr<ID3D12PipelineState> GfxPipelineState;
	ComPtr<ID3D12PipelineState> CmpPipelineState;

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

	void GfxCommandsBegin(ID3D12PipelineState* PipelineState = nullptr);
	void GfxCommandsEnd();

	void CmpCommandsBegin(ID3D12PipelineState* PipelineState = nullptr);
	void CmpCommandsEnd();

	void CreateGraphicsAndComputePipeline(ID3D12RootSignature* GfxRootSignature, ID3D12RootSignature* CmpRootSignature);
	void BeginRender(ID3D12RootSignature* RootSignature, const buffer& Buffer, const buffer& Buffer1);
	void Draw();
	void DrawIndexed();
	void DrawIndirect(ID3D12CommandSignature* CommandSignature, const buffer& VertexBuffer, const buffer& IndexBuffer, u32 DrawCount, const buffer& IndirectCommands);
	void EndRender(const buffer& Buffer);

	void BeginCompute(ID3D12RootSignature* RootSignature, const buffer& Buffer0, const buffer& Buffer1, const buffer& Buffer2, const buffer& Buffer3, const buffer& Buffer4);
	void Dispatch(u32 X, u32 Y, u32 Z = 1);
	void EndCompute(const buffer& Buffer0, const buffer& Buffer1);

	void FenceSignal(ID3D12CommandQueue* CommandQueue);
	void FenceWait();
	void GfxFlush();
	void CmpFlush();

	u32 Width;
	u32 Height;

	double RefreshRate;

	ComPtr<ID3D12Device6> Device;
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
