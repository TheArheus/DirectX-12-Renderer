
renderer_backend::
renderer_backend(HWND Window, u32 ClientWidth, u32 ClientHeight)
{
	Viewport = { 0, 0, (r32)ClientWidth, (r32)ClientHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
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
		SwapChainDesc.Width = ClientWidth;
		SwapChainDesc.Height = ClientHeight;
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

	GfxCommandAlloc->Reset();
	GfxCommandList->Reset(GfxCommandAlloc.Get(), nullptr);
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
		ResourceHeapDesc.NumDescriptors = 64;
		ResourceHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ResourceHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		Device->CreateDescriptorHeap(&ResourceHeapDesc, IID_PPV_ARGS(&GfxResourceHeap));
		ResourceHeapDesc.NumDescriptors = 64;
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
		DepthStencilDesc.Width = ClientWidth;
		DepthStencilDesc.Height = ClientHeight;
		DepthStencilDesc.DepthOrArraySize = 1;
		DepthStencilDesc.MipLevels = 1;
		DepthStencilDesc.Format = DepthBufferFormat;
		DepthStencilDesc.SampleDesc.Count = MsaaState ? 4 : 1;
		DepthStencilDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
		DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE Clear = {};
		Clear.Format = DepthBufferFormat;
		Clear.DepthStencil.Depth = 0.0f;
		Clear.DepthStencil.Stencil = 0;
		auto HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		Device->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&DepthStencilBuffer.Handle));

		Device->CreateDepthStencilView(DepthStencilBuffer.Handle.Get(), nullptr, DsvHeap->GetCPUDescriptorHandleForHeapStart());

		//auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		//GfxCommandList->ResourceBarrier(1, &Barrier);
	}
	GfxCommandList->Close();
	ID3D12CommandList* GfxCmdLists[] = { GfxCommandList.Get() };
	GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);
	Flush(GfxCommandQueue.Get());
};

ID3D12PipelineState* renderer_backend::
CreateGraphicsPipeline(ID3D12RootSignature* GfxRootSignature, std::initializer_list<const std::string> ShaderList)
{
	D3D12_INPUT_ELEMENT_DESC InputDesc[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
		{"TEXTCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
		{"NORMAL", 0, DXGI_FORMAT_R32_UINT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
	};

	D3D12_RASTERIZER_DESC RasterDesc = {};
	RasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
	RasterDesc.CullMode = D3D12_CULL_MODE_BACK;
	RasterDesc.FrontCounterClockwise = true;
	RasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	RasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	RasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	RasterDesc.DepthClipEnable = true;
	RasterDesc.MultisampleEnable = false;
	RasterDesc.AntialiasedLineEnable = false;
	RasterDesc.ForcedSampleCount = 0;
	RasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = {};
	DepthStencilDesc.DepthEnable = true;
	DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	DepthStencilDesc.StencilEnable = false;
	DepthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	DepthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	DepthStencilDesc.FrontFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
	DepthStencilDesc.BackFace = {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineDesc = {};
	PipelineDesc.InputLayout = {InputDesc, _countof(InputDesc)};
	PipelineDesc.pRootSignature = GfxRootSignature;

	std::vector<ComPtr<ID3DBlob>> ShadersBlob;
	for(const std::string Shader : ShaderList)
	{
		ID3DBlob* ShaderBlob;
		D3DReadFileToBlob(std::wstring(Shader.begin(), Shader.end()).c_str(), &ShaderBlob);

		if(Shader.find(".vert.") != std::string::npos)
		{
			PipelineDesc.VS = CD3DX12_SHADER_BYTECODE(ShaderBlob);
			ShadersBlob.push_back(ComPtr<ID3DBlob>(ShaderBlob));
		}
		if(Shader.find(".doma.") != std::string::npos)
		{
			PipelineDesc.DS = CD3DX12_SHADER_BYTECODE(ShaderBlob);
			ShadersBlob.push_back(ComPtr<ID3DBlob>(ShaderBlob));
		}
		if(Shader.find(".hull.") != std::string::npos)
		{
			PipelineDesc.HS = CD3DX12_SHADER_BYTECODE(ShaderBlob);
			ShadersBlob.push_back(ComPtr<ID3DBlob>(ShaderBlob));
		}
		if (Shader.find(".geom.") != std::string::npos)
		{
			PipelineDesc.GS = CD3DX12_SHADER_BYTECODE(ShaderBlob);
			ShadersBlob.push_back(ComPtr<ID3DBlob>(ShaderBlob));
		}
		if(Shader.find(".frag.") != std::string::npos)
		{
			PipelineDesc.PS = CD3DX12_SHADER_BYTECODE(ShaderBlob);
			ShadersBlob.push_back(ComPtr<ID3DBlob>(ShaderBlob));
		}
	}

	PipelineDesc.RasterizerState = RasterDesc;
	PipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	PipelineDesc.DepthStencilState = DepthStencilDesc;
	PipelineDesc.SampleMask = UINT_MAX;
	PipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PipelineDesc.NumRenderTargets = 1;
	PipelineDesc.SampleDesc.Count = MsaaState ? 4 : 1;
	PipelineDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
	PipelineDesc.RTVFormats[0] = BackBufferFormat;
	PipelineDesc.DSVFormat = DepthBufferFormat;
	PipelineDesc.SampleDesc.Count = 1;
	ID3D12PipelineState* Result;
	Device->CreateGraphicsPipelineState(&PipelineDesc, IID_PPV_ARGS(&Result));

	return Result;
}

ID3D12PipelineState* renderer_backend::
CreateComputePipeline(ID3D12RootSignature* CmpRootSignature, const std::string& ComputeShader)
{
	ComPtr<ID3DBlob> CompShader;
	D3DReadFileToBlob(std::wstring(ComputeShader.begin(), ComputeShader.end()).c_str(), &CompShader);

	D3D12_COMPUTE_PIPELINE_STATE_DESC PipelineDesc = {};
	PipelineDesc.CS = CD3DX12_SHADER_BYTECODE(CompShader.Get());
	PipelineDesc.pRootSignature = CmpRootSignature;
	ID3D12PipelineState* Result;
	Device->CreateComputePipelineState(&PipelineDesc, IID_PPV_ARGS(&Result));

	return Result;
}

void renderer_backend::
Flush(ID3D12CommandQueue* CommandQueue)
{
	FenceSignal(CommandQueue);
	FenceWait();
}

void renderer_backend::
FenceSignal(ID3D12CommandQueue* CommandQueue)
{
	CommandQueue->Signal(Fence.Get(), ++CurrentFence);
}

void renderer_backend::
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

void renderer_backend::
RecreateSwapchain(u32 NewWidth, u32 NewHeight)
{
	Flush(GfxCommandQueue.Get());

	Viewport.Width  = (r32)NewWidth;
	Viewport.Height = (r32)NewHeight;
	Rect = { 0, 0, (LONG)NewWidth, (LONG)NewHeight };

	GfxCommandAlloc->Reset();
	GfxCommandList->Reset(GfxCommandAlloc.Get(), nullptr);

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
	DepthStencilDesc.Width = NewWidth;
	DepthStencilDesc.Height = NewHeight;
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

	//auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Handle.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//GfxCommandList->ResourceBarrier(1, &Barrier);

	GfxCommandList->Close();
	ID3D12CommandList* GfxCmdLists[] = { GfxCommandList.Get() };
	GfxCommandQueue->ExecuteCommandLists(_countof(GfxCmdLists), GfxCmdLists);

	Flush(GfxCommandQueue.Get());
}

#if 0
void renderer_backend::
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

void renderer_backend::
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

void renderer_backend::
Draw()
{
	for(const vertex_buffer_view& View : VertexViews)
	{
		GfxCommandList->IASetVertexBuffers(0, 1, &View.Handle);
		GfxCommandList->DrawInstanced(View.VertexCount, 1, View.VertexBegin, 0);
	}
}

void renderer_backend::
DrawIndexed()
{
	for(const std::pair<vertex_buffer_view, index_buffer_view>& View : IndexedVertexViews)
	{
		GfxCommandList->IASetVertexBuffers(0, 1, &View.first.Handle);
		GfxCommandList->IASetIndexBuffer(&View.second.Handle);
		GfxCommandList->DrawIndexedInstanced(View.second.IndexCount, 1, View.second.IndexBegin, 0, 0);
	}
}
#endif

