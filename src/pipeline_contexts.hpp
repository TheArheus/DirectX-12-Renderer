#pragma once


class render_context
{
public:
	render_context(std::unique_ptr<renderer_backend>& Context, 
				   const shader_input& Signature, 
				   const buffer& VertexBuffer, const buffer& IndexBuffer,
				   std::initializer_list<const std::string> ShaderList,
				   const indirect_command_signature& IndirectSignature = {}):
		InputSignature(Signature),
		IndirectInputSignature(IndirectSignature)
	{
		Pipeline = Context->CreateGraphicsPipeline(Signature.Handle.Get(), ShaderList);
		CommandList = Context->GfxCommandQueue.AllocateCommandList();
		Fence.Init(Context->Device.Get());

		VertexBufferView = {VertexBuffer.GpuPtr, static_cast<u32>(VertexBuffer.Size), sizeof(vertex)};
		IndexBufferView  = {IndexBuffer.GpuPtr, static_cast<u32>(IndexBuffer.Size), DXGI_FORMAT_R32_UINT};
	}

	void Begin(command_queue& CommandQueue, ID3D12DescriptorHeap* ResourceHeap = nullptr, ID3D12DescriptorHeap* SamplerHeap = nullptr)
	{
		CommandQueue.Reset();
		CommandList->Reset(CommandQueue.CommandAlloc.Get(), Pipeline.Get());

		CommandList->SetGraphicsRootSignature(InputSignature.Handle.Get());
		CommandList->SetPipelineState(Pipeline.Get());

		u32 DescriptorHeapCount = (ResourceHeap != nullptr) + (SamplerHeap != nullptr);
		ID3D12DescriptorHeap* Heaps[] = { ResourceHeap, SamplerHeap };
		CommandList->SetDescriptorHeaps(DescriptorHeapCount, Heaps);

		CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void SetTargetAndClear(D3D12_VIEWPORT* Viewport, D3D12_RECT* Scissors, D3D12_CPU_DESCRIPTOR_HANDLE* RenderTarget, D3D12_CPU_DESCRIPTOR_HANDLE* DepthTarget)
	{
		CommandList->RSSetViewports(1, Viewport);
		CommandList->RSSetScissorRects(1, Scissors);

		CommandList->OMSetRenderTargets(1, RenderTarget, true, DepthTarget);

		float ClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		CommandList->ClearRenderTargetView(*RenderTarget, ClearColor, 0, nullptr);
		CommandList->ClearDepthStencilView(*DepthTarget, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);
	}

	void SetTarget(D3D12_VIEWPORT* Viewport, D3D12_RECT* Scissors, D3D12_CPU_DESCRIPTOR_HANDLE* RenderTarget, D3D12_CPU_DESCRIPTOR_HANDLE* DepthTarget)
	{
		CommandList->RSSetViewports(1, Viewport);
		CommandList->RSSetScissorRects(1, Scissors);

		CommandList->OMSetRenderTargets(1, RenderTarget, true, DepthTarget);
	}
	
	void End(command_queue& CommandQueue)
	{
		CommandQueue.Execute(CommandList);
		Fence.Flush(CommandQueue);
	}

	void Present(command_queue& CommandQueue, IDXGISwapChain4* SwapChain)
	{
		CommandQueue.Execute(CommandList);
		SwapChain->Present(0, 0);
		BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
		Fence.Flush(CommandQueue);
	}

	void Draw()
	{
		for(const vertex_buffer_view& View : VertexViews)
		{
			CommandList->IASetVertexBuffers(0, 1, &View.Handle);
			CommandList->DrawInstanced(View.VertexCount, 1, View.VertexBegin, 0);
		}
	}

	void DrawIndexed()
	{
		for(const std::pair<vertex_buffer_view, index_buffer_view>& View : IndexedVertexViews)
		{
			CommandList->IASetVertexBuffers(0, 1, &View.first.Handle);
			CommandList->IASetIndexBuffer(&View.second.Handle);
			CommandList->DrawIndexedInstanced(View.second.IndexCount, 1, View.second.IndexBegin, 0, 0);
		}
	}

	void DrawIndirect(u32 ObjectDrawCount, const buffer& IndirectCommands)
	{
		CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		CommandList->IASetIndexBuffer(&IndexBufferView);

		CommandList->ExecuteIndirect(IndirectInputSignature.Handle.Get(), ObjectDrawCount, IndirectCommands.Handle.Get(), 0, IndirectCommands.Handle.Get(), IndirectCommands.CounterOffset);
	}

	void SetDescriptorTable(u32 Idx, heap_alloc Heap, u32 StartFrom = 0)
	{
		CommandList->SetGraphicsRootDescriptorTable(Idx, Heap.GetGpuHandle(StartFrom));
	}

	template<typename T>
	void SetShaderResourceView(u32 Idx, const T& Buffer)
	{
		CommandList->SetGraphicsRootShaderResourceView(Idx, Buffer.GpuPtr);
	}

	template<typename T>
	void SetConstantBufferView(u32 Idx, const T& Buffer)
	{
		CommandList->SetGraphicsRootConstantBufferView(Idx, Buffer.GpuPtr);
	}

	template<typename T>
	void SetUnorderedAccessView(u32 Idx, const T& Buffer)
	{
		CommandList->SetGraphicsRootUnorderedAccessView(Idx, Buffer.GpuPtr);
	}

	ID3D12GraphicsCommandList* CommandList;
	u32 BackBufferIndex = 0;

private:

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	std::vector<vertex_buffer_view> VertexViews;
	std::vector<std::pair<vertex_buffer_view, index_buffer_view>> IndexedVertexViews;

	fence Fence;
	const shader_input& InputSignature;
	const indirect_command_signature& IndirectInputSignature;

	ComPtr<ID3D12PipelineState> Pipeline;
};

class compute_context
{
public:
	compute_context(std::unique_ptr<renderer_backend>& Context, const shader_input& Signature, const std::string& Shader):
		InputSignature(Signature)
	{
		Pipeline = Context->CreateComputePipeline(Signature.Handle.Get(), Shader);
		CommandList = Context->CmpCommandQueue.AllocateCommandList();
		Fence.Init(Context->Device.Get());
	}

	void Begin(command_queue& CommandQueue, ID3D12DescriptorHeap* ResourceHeap = nullptr, ID3D12DescriptorHeap* SamplerHeap = nullptr)
	{
		CommandQueue.Reset();
		CommandList->Reset(CommandQueue.CommandAlloc.Get(), Pipeline.Get());

		CommandList->SetComputeRootSignature(InputSignature.Handle.Get());
		CommandList->SetPipelineState(Pipeline.Get());

		u32 DescriptorHeapCount = (ResourceHeap != nullptr) + (SamplerHeap != nullptr);
		ID3D12DescriptorHeap* Heaps[] = { ResourceHeap, SamplerHeap };
		CommandList->SetDescriptorHeaps(DescriptorHeapCount, Heaps);
	}

	void End(command_queue& CommandQueue)
	{
		CommandQueue.Execute(CommandList);
		Fence.Flush(CommandQueue);
	}

	void Execute(u32 X, u32 Y, u32 Z)
	{
		CommandList->Dispatch((X + 31) / 32, (Y + 31) / 32, (Z + 31) / 32);
	}

	void SetDescriptorTable(u32 Idx, heap_alloc Heap, u32 StartFrom = 0)
	{
		CommandList->SetComputeRootDescriptorTable(Idx, Heap.GetGpuHandle(StartFrom));
	}

	template<typename T>
	void SetShaderResourceView(u32 Idx, const T& Buffer)
	{
		CommandList->SetComputeRootShaderResourceView(Idx, Buffer.GpuPtr);
	}

	template<typename T>
	void SetConstantBufferView(u32 Idx, const T& Buffer)
	{
		CommandList->SetComputeRootConstantBufferView(Idx, Buffer.GpuPtr);
	}

	template<typename T>
	void SetUnorderedAccessView(u32 Idx, const T& Buffer)
	{
		CommandList->SetComputeRootUnorderedAccessView(Idx, Buffer.GpuPtr);
	}

	ID3D12GraphicsCommandList* CommandList;

private:
	fence Fence;
	const shader_input& InputSignature;

	ComPtr<ID3D12PipelineState> Pipeline;
};

