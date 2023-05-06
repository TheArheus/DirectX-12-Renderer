
renderer_backend::
renderer_backend(HWND Window, u32 ClientWidth, u32 ClientHeight)
{
	Viewport = { 0, 0, (r32)ClientWidth, (r32)ClientHeight, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	Rect = { 0, 0, (LONG)ClientWidth, (LONG)ClientHeight };

	Width = ClientWidth;
	Height = ClientHeight;

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

	Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &TearingSupport, sizeof(TearingSupport));

	D3D12_FEATURE_DATA_FORMAT_SUPPORT DataFormatSupport = {BackBufferFormat, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};
	Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &DataFormatSupport, sizeof(DataFormatSupport));

	u32 RenderFormatRequired = D3D12_FORMAT_SUPPORT1_RENDER_TARGET | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET;
	bool RenderMultisampleSupport = (DataFormatSupport.Support1 & RenderFormatRequired) == RenderFormatRequired;
	u32 DepthFormatRequired = D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET;
	bool DepthMultisampleSupport = (DataFormatSupport.Support1 & DepthFormatRequired) == DepthFormatRequired;

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS MsQualityLevels = {};
	MsQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	MsQualityLevels.Format = BackBufferFormat;
	for (MsaaQuality = 4;
		 MsaaQuality > 1;
		 MsaaQuality--)
	{
		MsQualityLevels.SampleCount = MsaaQuality;
		if (FAILED(Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &MsQualityLevels, sizeof(MsQualityLevels)))) continue;
		if (MsQualityLevels.NumQualityLevels > 0) break;
	}
	
	MsaaQuality = MsQualityLevels.NumQualityLevels;
	if (MsaaQuality >= 2) MsaaState = true;
	if (MsaaQuality < 2) RenderMultisampleSupport = false;

	GfxCommandQueue.Init(Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	CmpCommandQueue.Init(Device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
	Fence.Init(Device.Get());

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
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.SampleDesc.Quality = 0;
		Factory->CreateSwapChainForHwnd(GfxCommandQueue.Handle.Get(), Window, &SwapChainDesc, nullptr, nullptr, _SwapChain.GetAddressOf());
		_SwapChain.As(&SwapChain);
	}

	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	{
		RtvHeap.Init(Device.Get(), 2, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		DsvHeap.Init(Device.Get(), 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		ResourceHeap.Init(Device.Get(), 128, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		heap_alloc RtvHeapMap = RtvHeap.Allocate(2);
		heap_alloc DsvHeapMap = DsvHeap.Allocate(1);
		RtvHeap.Reset();
		DsvHeap.Reset();

		D3D12_RESOURCE_DESC RenderTargetDesc = {};
		RenderTargetDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		RenderTargetDesc.Width = ClientWidth;
		RenderTargetDesc.Height = ClientHeight;
		RenderTargetDesc.DepthOrArraySize = 1;
		RenderTargetDesc.MipLevels = 1;
		RenderTargetDesc.Format = BackBufferFormat;
		RenderTargetDesc.SampleDesc.Count = RenderMultisampleSupport ? MsaaQuality : 1;
		RenderTargetDesc.SampleDesc.Quality = RenderMultisampleSupport ? (MsaaQuality - 1) : 0;
		RenderTargetDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		RenderTargetDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		float RenderTargetClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		D3D12_CLEAR_VALUE Clear = {};
		Clear.Format = BackBufferFormat;
		memcpy(Clear.Color, RenderTargetClearColor, sizeof(float)*4);
		auto RenderTargetHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		//Device->CreateCommittedResource(&RenderTargetHeapProp, D3D12_HEAP_FLAG_NONE, &RenderTargetDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&BackBuffers[0].Handle));
		//Device->CreateCommittedResource(&RenderTargetHeapProp, D3D12_HEAP_FLAG_NONE, &RenderTargetDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&BackBuffers[1].Handle));

		D3D12_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc = {};
		RenderTargetViewDesc.Format = BackBufferFormat;
		RenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

		for (u32 Idx = 0;
			Idx < 2;
			++Idx)
		{
			SwapChain->GetBuffer(Idx, IID_PPV_ARGS(&BackBuffers[Idx].Handle));
			Device->CreateRenderTargetView(BackBuffers[Idx].Handle.Get(), &RenderTargetViewDesc, RtvHeapMap.GetNextCpuHandle());
		}

		D3D12_RESOURCE_DESC DepthStencilDesc = {};
		DepthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		DepthStencilDesc.Width = ClientWidth;
		DepthStencilDesc.Height = ClientHeight;
		DepthStencilDesc.DepthOrArraySize = 1;
		DepthStencilDesc.MipLevels = 1;
		DepthStencilDesc.Format = DepthBufferFormat;
		DepthStencilDesc.SampleDesc.Count = DepthMultisampleSupport ? MsaaQuality : 1;
		DepthStencilDesc.SampleDesc.Quality = DepthMultisampleSupport ? (MsaaQuality - 1) : 0;
		DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		Clear.Format = DepthBufferFormat;
		Clear.DepthStencil.Depth = 0.0f;
		Clear.DepthStencil.Stencil = 0;
		auto DepthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		Device->CreateCommittedResource(&DepthHeapProp, D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&DepthStencilBuffer.Handle));

		Device->CreateDepthStencilView(DepthStencilBuffer.Handle.Get(), nullptr, DsvHeapMap.GetNextCpuHandle());
	}
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
	RasterDesc.MultisampleEnable = MsaaState;
	RasterDesc.AntialiasedLineEnable = MsaaState;
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
	PipelineDesc.SampleDesc.Count = MsaaState ? MsaaQuality : 1;
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
RecreateSwapchain(u32 NewWidth, u32 NewHeight)
{
	Fence.Flush(GfxCommandQueue);
	ID3D12GraphicsCommandList* CommandList = GfxCommandQueue.AllocateCommandList();

	heap_alloc RtvHeapMap = RtvHeap.Allocate(2);
	heap_alloc DsvHeapMap = DsvHeap.Allocate(1);
	RtvHeap.Reset();
	DsvHeap.Reset();

	Width  = NewWidth;
	Height = NewHeight;
	Viewport.Width  = (r32)NewWidth;
	Viewport.Height = (r32)NewHeight;
	Rect = { 0, 0, (LONG)NewWidth, (LONG)NewHeight };

	GfxCommandQueue.Reset();
	CommandList->Reset(GfxCommandQueue.CommandAlloc.Get(), nullptr);

	for (u32 Idx = 0;
		Idx < 2;
		++Idx)
	{
		BackBuffers[Idx].Handle.Reset();
	}
	DepthStencilBuffer.Handle.Reset();

	SwapChain->ResizeBuffers(2, NewWidth, NewHeight, BackBufferFormat, (TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	BackBufferIndex = 0;

	for (u32 Idx = 0;
		Idx < 2;
		++Idx)
	{
		SwapChain->GetBuffer(Idx, IID_PPV_ARGS(&BackBuffers[Idx].Handle));
		Device->CreateRenderTargetView(BackBuffers[Idx].Handle.Get(), nullptr, RtvHeapMap.GetNextCpuHandle());
	}

	D3D12_RESOURCE_DESC DepthStencilDesc = {};
	DepthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	DepthStencilDesc.Width = NewWidth;
	DepthStencilDesc.Height = NewHeight;
	DepthStencilDesc.DepthOrArraySize = 1;
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.Format = DepthBufferFormat;
	DepthStencilDesc.SampleDesc.Count = MsaaState ? MsaaQuality : 1;
	DepthStencilDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
	DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE Clear = {};
	Clear.Format = DepthBufferFormat;
	Clear.DepthStencil.Depth = 1.0f;
	Clear.DepthStencil.Stencil = 0;
	auto HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	Device->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&DepthStencilBuffer.Handle));

	Device->CreateDepthStencilView(DepthStencilBuffer.Handle.Get(), nullptr, DsvHeapMap.GetNextCpuHandle());

	GfxCommandQueue.ExecuteAndRemove(CommandList);
	Fence.Flush(GfxCommandQueue);
}

void renderer_backend::
Present()
{
	SwapChain->Present(0, 0);
	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
}

