
d3d_app::
d3d_app(HWND Window, u32 ClientWidth, u32 ClientHeight) : Width(ClientWidth), Height(ClientHeight)
{
	Viewport = { 0, 0, (r32)Width, (r32)Height, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	Rect = { 0, 0, (LONG)ClientWidth, (LONG)ClientHeight };

#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> Debug;
		D3D12GetDebugInterface(IID_PPV_ARGS(&Debug));
		Debug->EnableDebugLayer();
	}
#endif
	
	CreateDXGIFactory1(IID_PPV_ARGS(&Factory));

	if (UseWarp)
	{
		ComPtr<IDXGIAdapter> WarpAdapter;
		Factory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter));
		D3D12CreateDevice(WarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
	}
	else
	{
		GetDevice(&Device);
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

	D3D12_COMMAND_QUEUE_DESC GfxCommandQueueDesc = {};
	GfxCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	GfxCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	Device->CreateCommandQueue(&GfxCommandQueueDesc, IID_PPV_ARGS(&GfxCommandQueue));
	D3D12_COMMAND_QUEUE_DESC CmpCommandQueueDesc = {};
	CmpCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	CmpCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	Device->CreateCommandQueue(&CmpCommandQueueDesc, IID_PPV_ARGS(&CmpCommandQueue));

	Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&GfxCommandAlloc));
	Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&CmpCommandAlloc));

	Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, GfxCommandAlloc.Get(), nullptr, IID_PPV_ARGS(&GfxCommandList));
	Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, CmpCommandAlloc.Get(), nullptr, IID_PPV_ARGS(&CmpCommandList));
	GfxCommandList->Close();
	CmpCommandList->Close();

	{
		ComPtr<IDXGISwapChain1> _SwapChain;
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Flags = (TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		SwapChainDesc.Width = Width;
		SwapChainDesc.Height = Height;
		SwapChainDesc.BufferCount = 2;
		SwapChainDesc.Format = BackBufferFormat;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count = MsaaState ? 4 : 1;
		SwapChainDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
		Factory->CreateSwapChainForHwnd(GfxCommandQueue.Get(), Window, &SwapChainDesc, nullptr, nullptr, _SwapChain.GetAddressOf());
		_SwapChain.As(&SwapChain);
	}

	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	GfxCommandsBegin();
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

		D3D12_DESCRIPTOR_HEAP_DESC ResourceHeapDesc = {};
		ResourceHeapDesc.NumDescriptors = 1;
		ResourceHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		Device->CreateDescriptorHeap(&ResourceHeapDesc, IID_PPV_ARGS(&GfxResourceHeap));
		ResourceHeapDesc.NumDescriptors = 4;
		ResourceHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		Device->CreateDescriptorHeap(&ResourceHeapDesc, IID_PPV_ARGS(&CmpResourceHeap));
		ResourceHeapIncrement = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHeapHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (u32 Idx = 0;
			Idx < 2;
			++Idx)
		{
			SwapChain->GetBuffer(Idx, IID_PPV_ARGS(&BackBuffers[Idx].Handle));
			Device->CreateRenderTargetView(BackBuffers[Idx].Handle.Get(), nullptr, RtvHeapHandle);
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
		Device->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&DepthStencilBuffer.Handle));

		Device->CreateDepthStencilView(DepthStencilBuffer.Handle.Get(), nullptr, DsvHeap->GetCPUDescriptorHandleForHeapStart());

		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		GfxCommandList->ResourceBarrier(1, &Barrier);
	}
	GfxCommandsEnd();
	GfxFlush();
};

void d3d_app::
CreateGraphicsAndComputePipeline(ID3D12RootSignature* GfxRootSignature, ID3D12RootSignature* CmpRootSignature)
{
	{
		{
			ComPtr<ID3DBlob> VertShader;
			ComPtr<ID3DBlob> FragShader;
			ComPtr<ID3DBlob> CompShader;
			D3DReadFileToBlob(L"..\\build\\mesh.vert.cso", &VertShader);
			D3DReadFileToBlob(L"..\\build\\mesh.frag.cso", &FragShader);
			D3DReadFileToBlob(L"..\\build\\indirect.comp.cso", &CompShader);

			D3D12_INPUT_ELEMENT_DESC InputDesc[] = 
			{
				{"POSITION", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
				{"TEXTCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
				{"NORMAL", 0, DXGI_FORMAT_R32_UINT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
			};

			D3D12_RASTERIZER_DESC RasterDesc = {};
			RasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
			RasterDesc.CullMode = D3D12_CULL_MODE_BACK;
			RasterDesc.FrontCounterClockwise = FALSE;
			RasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			RasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			RasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			RasterDesc.DepthClipEnable = FALSE;
			RasterDesc.MultisampleEnable = FALSE;
			RasterDesc.AntialiasedLineEnable = FALSE;
			RasterDesc.ForcedSampleCount = 0;
			RasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineDesc = {};
			PipelineDesc.InputLayout = {InputDesc, _countof(InputDesc)};
			PipelineDesc.pRootSignature = GfxRootSignature;
			PipelineDesc.VS = CD3DX12_SHADER_BYTECODE(VertShader.Get());
			PipelineDesc.PS = CD3DX12_SHADER_BYTECODE(FragShader.Get());
			PipelineDesc.RasterizerState = RasterDesc;
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
			Device->CreateGraphicsPipelineState(&PipelineDesc, IID_PPV_ARGS(&GfxPipelineState));

			D3D12_COMPUTE_PIPELINE_STATE_DESC CmpPipelineDesc = {};
			CmpPipelineDesc.CS = CD3DX12_SHADER_BYTECODE(CompShader.Get());
			CmpPipelineDesc.pRootSignature = CmpRootSignature;
			Device->CreateComputePipelineState(&CmpPipelineDesc, IID_PPV_ARGS(&CmpPipelineState));
		}
	}
}

void d3d_app::BeginRender(ID3D12RootSignature* RootSignature, const buffer& Buffer)
{
	GfxCommandsBegin(GfxPipelineState.Get());

	GfxCommandList->SetGraphicsRootSignature(RootSignature);
	GfxCommandList->SetPipelineState(GfxPipelineState.Get());

	ID3D12DescriptorHeap* Heaps[] = {GfxResourceHeap.Get()};
	GfxCommandList->SetDescriptorHeaps(1, Heaps);

	CD3DX12_RESOURCE_BARRIER Barrier[] = 
	{
		CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIndex].Handle.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
		CD3DX12_RESOURCE_BARRIER::Transition(Buffer.Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
	};
	GfxCommandList->ResourceBarrier(2, Barrier);

	GfxCommandList->RSSetViewports(1, &Viewport);
	GfxCommandList->RSSetScissorRects(1, &Rect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE RenderViewHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart(), BackBufferIndex, RtvSize);
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView = DsvHeap->GetCPUDescriptorHandleForHeapStart();
	GfxCommandList->OMSetRenderTargets(1, &RenderViewHandle, false, &DepthStencilView);

	float ClearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GfxCommandList->ClearRenderTargetView(RenderViewHandle, ClearColor, 0, nullptr);
	GfxCommandList->ClearDepthStencilView(DsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	GfxCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
};

void d3d_app::
Draw()
{
	for(const vertex_buffer_view& View : VertexViews)
	{
		GfxCommandList->IASetVertexBuffers(0, 1, &View.Handle);
		GfxCommandList->DrawInstanced(View.VertexCount, 1, View.VertexBegin, 0);
	}
}

void d3d_app::
DrawIndexed()
{
	for(const std::pair<vertex_buffer_view, index_buffer_view>& View : IndexedVertexViews)
	{
		GfxCommandList->IASetVertexBuffers(0, 1, &View.first.Handle);
		GfxCommandList->IASetIndexBuffer(&View.second.Handle);
		GfxCommandList->DrawIndexedInstanced(View.second.IndexCount, 1, View.second.IndexBegin, 0, 0);
	}
}

void d3d_app::
DrawIndirect(ID3D12CommandSignature* CommandSignature, const buffer& VertexBuffer, const buffer& IndexBuffer, u32 DrawCount, buffer IndirectCommands)
{
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {VertexBuffer.GpuPtr, static_cast<u32>(VertexBuffer.Size), sizeof(vertex)};
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {IndexBuffer.GpuPtr, static_cast<u32>(IndexBuffer.Size), DXGI_FORMAT_R32_UINT};
	GfxCommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
	GfxCommandList->IASetIndexBuffer(&IndexBufferView);
	GfxCommandList->ExecuteIndirect(CommandSignature, DrawCount, IndirectCommands.Handle.Get(), 0, nullptr, 0);
}

void d3d_app::
EndRender(const buffer& Buffer)
{
	CD3DX12_RESOURCE_BARRIER Barrier[] = 
	{
		CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIndex].Handle.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
		CD3DX12_RESOURCE_BARRIER::Transition(Buffer.Handle.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COMMON),
	};
	GfxCommandList->ResourceBarrier(2, Barrier);

	GfxCommandsEnd();

	SwapChain->Present(0, 0);

	GfxFlush();
	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
}

void d3d_app::
BeginCompute(ID3D12RootSignature* RootSignature, buffer Buffer0, buffer Buffer1, buffer Buffer2, buffer Buffer3)
{
	CmpCommandsBegin(CmpPipelineState.Get());

	CmpCommandList->SetComputeRootSignature(RootSignature);
	CmpCommandList->SetPipelineState(CmpPipelineState.Get());

	ID3D12DescriptorHeap* Heaps[] = { CmpResourceHeap.Get() };
	CmpCommandList->SetDescriptorHeaps(1, Heaps);

	CmpCommandList->SetComputeRootShaderResourceView(0, Buffer0.GpuPtr);
	CmpCommandList->SetComputeRootShaderResourceView(1, Buffer1.GpuPtr);
	CmpCommandList->SetComputeRootUnorderedAccessView(2, Buffer2.GpuPtr);
	CmpCommandList->SetComputeRootUnorderedAccessView(3, Buffer3.GpuPtr);

	D3D12_RESOURCE_BARRIER barrier[] = 
	{
		CD3DX12_RESOURCE_BARRIER::Transition(Buffer2.Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(Buffer3.Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
	};
	CmpCommandList->ResourceBarrier(2, barrier);
}

void d3d_app::
Dispatch(u32 X, u32 Y, u32 Z)
{
	CmpCommandList->Dispatch(X, Y, Z);
}

void d3d_app::
EndCompute(buffer Buffer0, buffer Buffer1)
{
	D3D12_RESOURCE_BARRIER barrier[] = 
	{
		CD3DX12_RESOURCE_BARRIER::Transition(Buffer0.Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON),
		CD3DX12_RESOURCE_BARRIER::Transition(Buffer1.Handle.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON),
	};
	CmpCommandList->ResourceBarrier(2, barrier);

	CmpCommandsEnd();
	CmpFlush();
}

void d3d_app::
GfxFlush()
{
	FenceSignal(GfxCommandQueue.Get());
	FenceWait();
}

void d3d_app::
CmpFlush()
{
	FenceSignal(CmpCommandQueue.Get());
	FenceWait();
}

void d3d_app::
FenceSignal(ID3D12CommandQueue* CommandQueue)
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
GfxCommandsBegin(ID3D12PipelineState* _PipelineState)
{
	GfxCommandAlloc->Reset();
	GfxCommandList->Reset(GfxCommandAlloc.Get(), _PipelineState);
}

void d3d_app::
GfxCommandsEnd()
{
	GfxCommandList->Close();
	ID3D12CommandList* CmdLists[] = { GfxCommandList.Get() };
	GfxCommandQueue->ExecuteCommandLists(_countof(CmdLists), CmdLists);
}

void d3d_app::
CmpCommandsBegin(ID3D12PipelineState* _PipelineState)
{
	CmpCommandAlloc->Reset();
	CmpCommandList->Reset(CmpCommandAlloc.Get(), _PipelineState);
}

void d3d_app::
CmpCommandsEnd()
{
	CmpCommandList->Close();
	ID3D12CommandList* CmdLists[] = { CmpCommandList.Get() };
	CmpCommandQueue->ExecuteCommandLists(_countof(CmdLists), CmdLists);
}

void d3d_app::
RecreateSwapchain(u32 NewWidth, u32 NewHeight)
{
	GfxFlush();
	Width = NewWidth;
	Height = NewHeight;

	Viewport = { 0, 0, (r32)NewWidth, (r32)NewHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	Rect = { 0, 0, (LONG)NewWidth, (LONG)NewHeight };

	GfxCommandsBegin();

	for (u32 Idx = 0;
		Idx < 2;
		++Idx)
	{
		BackBuffers[Idx].Handle.Reset();
	}
	DepthStencilBuffer.Handle.Reset();

	SwapChain->ResizeBuffers(2, NewWidth, NewHeight, BackBufferFormat, (TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	BackBufferIndex = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHeapHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (u32 Idx = 0;
		Idx < 2;
		++Idx)
	{
		SwapChain->GetBuffer(Idx, IID_PPV_ARGS(&BackBuffers[Idx].Handle));
		Device->CreateRenderTargetView(BackBuffers[Idx].Handle.Get(), nullptr, RtvHeapHandle);
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
	Device->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&DepthStencilBuffer.Handle));

	Device->CreateDepthStencilView(DepthStencilBuffer.Handle.Get(), nullptr, DsvHeap->GetCPUDescriptorHandleForHeapStart());

	auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	GfxCommandList->ResourceBarrier(1, &Barrier);

	GfxCommandsEnd();

	GfxFlush();
}

void d3d_app::
PushVertexData(buffer* VertexBuffer, std::vector<mesh::offset>& Offsets)
{
	vertex_buffer_view VertexBufferView = {};
	for(const mesh::offset& Offset : Offsets)
	{
		VertexBufferView.VertexBegin = Offset.VertexOffset;
		VertexBufferView.VertexCount = Offset.VertexCount;
		VertexBufferView.Handle = {VertexBuffer->GpuPtr, static_cast<u32>(VertexBuffer->Size), sizeof(vertex)};
		VertexViews.push_back(VertexBufferView);
	}
}

void d3d_app::
PushIndexedVertexData(buffer* VertexBuffer, buffer* IndexBuffer, std::vector<mesh::offset>& Offsets)
{
	vertex_buffer_view VertexBufferView = {};
	index_buffer_view IndexBufferView = {};
	for(const mesh::offset& Offset : Offsets)
	{
		VertexBufferView.VertexBegin = Offset.VertexOffset;
		VertexBufferView.VertexCount = Offset.VertexCount;
		VertexBufferView.Handle = {VertexBuffer->GpuPtr, static_cast<u32>(VertexBuffer->Size), sizeof(vertex)};

		IndexBufferView.IndexBegin = Offset.IndexOffset;
		IndexBufferView.IndexCount = Offset.IndexCount;
		IndexBufferView.Handle = {IndexBuffer->GpuPtr, static_cast<u32>(IndexBuffer->Size), DXGI_FORMAT_R32_UINT};

		IndexedVertexViews.push_back(std::make_pair(VertexBufferView, IndexBufferView));
	}
}

