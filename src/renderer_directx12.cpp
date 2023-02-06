
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

	HRESULT HardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
	if (FAILED(HardwareResult))
	{
		ComPtr<IDXGIAdapter> WarpAdapter;
		Factory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter));
		D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
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
		SwapChainDesc.Format = BackBufferFormat;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		SwapChainDesc.SampleDesc.Count = MsaaState ? 4 : 1;
		SwapChainDesc.SampleDesc.Quality = MsaaState ? (MsaaQuality - 1) : 0;
		Factory->CreateSwapChainForHwnd(CommandQueue.Get(), Window, &SwapChainDesc, nullptr, nullptr, _SwapChain.GetAddressOf());
		_SwapChain.As(&SwapChain);
	}
};

void d3d_app::OnInit(mesh& Mesh) 
{
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
		for (u32 Idx = 0;
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
		Device->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &Clear, IID_PPV_ARGS(&DepthStencilBuffer));

		Device->CreateDepthStencilView(DepthStencilBuffer.Get(), nullptr, DsvHeap->GetCPUDescriptorHandleForHeapStart());

		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		CommandList->ResourceBarrier(1, &Barrier);
		Flush();
	}

	{
		const u32 VertexBufferSize = Mesh.Vertices.size() * sizeof(vertex);

		auto TempBufferProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto TempBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
		Device->CreateCommittedResource(&TempBufferProp, D3D12_HEAP_FLAG_NONE, &TempBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer1));

		auto VertexBufferProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto VertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
		Device->CreateCommittedResource(&VertexBufferProp, D3D12_HEAP_FLAG_NONE, &VertexBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&VertexBuffer));

		D3D12_SUBRESOURCE_DATA SubresData = {};
		SubresData.pData = Mesh.Vertices.data();
		SubresData.RowPitch = VertexBufferSize;
		SubresData.SlicePitch = VertexBufferSize;
		
		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(VertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CommandList->ResourceBarrier(1, &Barrier);
		UpdateSubresources<1>(CommandList.Get(), VertexBuffer.Get(), TempBuffer1.Get(), 0, 0, 1, &SubresData);

		VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
		VertexBufferView.StrideInBytes = sizeof(vertex);
		VertexBufferView.SizeInBytes = VertexBufferSize;
	}

	{
		const u32 IndexBufferSize = Mesh.VertexIndices.size() * sizeof(u32);

		auto TempBufferProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto TempBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);
		Device->CreateCommittedResource(&TempBufferProp, D3D12_HEAP_FLAG_NONE, &TempBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&TempBuffer2));

		auto IndexBufferProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto IndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);
		Device->CreateCommittedResource(&IndexBufferProp, D3D12_HEAP_FLAG_NONE, &IndexBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&IndexBuffer));

		D3D12_SUBRESOURCE_DATA SubresData = {};
		SubresData.pData = Mesh.VertexIndices.data();
		SubresData.RowPitch = IndexBufferSize;
		SubresData.SlicePitch = IndexBufferSize;
		
		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(IndexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CommandList->ResourceBarrier(1, &Barrier);
		UpdateSubresources<1>(CommandList.Get(), IndexBuffer.Get(), TempBuffer2.Get(), 0, 0, 1, &SubresData);

		IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
		IndexBufferView.SizeInBytes = IndexBufferSize;
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	}
	CommandsEnd();
	Flush();

	{
		{
			CD3DX12_ROOT_PARAMETER RootParameter = {};
			RootParameter.InitAsUnorderedAccessView(0);
			CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
			RootSignatureDesc.Init(1, &RootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
				{"POSITION", 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
				{"TEXTCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
				{"NORMAL", 0, DXGI_FORMAT_R32_UINT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
			};

			D3D12_RASTERIZER_DESC RasterDesc = {};
			RasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
			RasterDesc.CullMode = D3D12_CULL_MODE_FRONT;
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
			PipelineDesc.pRootSignature = RootSignature.Get();
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
			Device->CreateGraphicsPipelineState(&PipelineDesc, IID_PPV_ARGS(&PipelineState));
		}
	}
}

void d3d_app::BeginRender()
{
	CommandsBegin(PipelineState.Get());

	CommandList->SetGraphicsRootSignature(RootSignature.Get());
	CommandList->SetPipelineState(PipelineState.Get());

	auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &Barrier);

	CommandList->RSSetViewports(1, &Viewport);
	CommandList->RSSetScissorRects(1, &Rect);

	CD3DX12_CPU_DESCRIPTOR_HANDLE RenderViewHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart(), BackBufferIndex, RtvSize);
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView = DsvHeap->GetCPUDescriptorHandleForHeapStart();
	CommandList->OMSetRenderTargets(1, &RenderViewHandle, false, &DepthStencilView);

	float ClearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	CommandList->ClearRenderTargetView(RenderViewHandle, ClearColor, 0, nullptr);
	CommandList->ClearDepthStencilView(DsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
};

void d3d_app::
Draw(mesh& Mesh)
{
	CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
	CommandList->DrawInstanced(Mesh.Vertices.size(), 1, 0, 0);
}

void d3d_app::
DrawIndexed(mesh& Mesh)
{
	CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
	CommandList->IASetIndexBuffer(&IndexBufferView);
	CommandList->DrawIndexedInstanced(Mesh.VertexIndices.size(), 1, 0, 0, 0);
}

void d3d_app::
EndRender()
{

	auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(BackBuffers[BackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->ResourceBarrier(1, &Barrier);

	CommandsEnd();

	SwapChain->Present(0, 0);

	Flush();
	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
}

void d3d_app::
Flush()
{
	FenceSignal();
	FenceWait();
}

void d3d_app::
FenceSignal()
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
CommandsBegin(ID3D12PipelineState* _PipelineState)
{
	CommandAlloc->Reset();
	CommandList->Reset(CommandAlloc.Get(), _PipelineState);
}

void d3d_app::
CommandsEnd()
{
	CommandList->Close();
	ID3D12CommandList* CmdLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CmdLists), CmdLists);
}
