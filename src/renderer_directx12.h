#ifndef RENDERER_DIRECTX_12_H_

using namespace Microsoft::WRL;
using namespace DirectX;

class d3d_app 
{
	u32 Width;
	u32 Height;

	u32 MsaaQuality;
	bool MsaaState = true;
	bool TearingSupport = false;
	bool UseWarp = false;

	D3D12_VIEWPORT Viewport;
	D3D12_RECT Rect;

	const DXGI_FORMAT BackBufferFormat  = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	ComPtr<IDXGIFactory6> Factory;
	ComPtr<ID3D12Device6> Device;
	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12CommandAllocator> CommandAlloc;
	ComPtr<ID3D12GraphicsCommandList> CommandList;
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

	ComPtr<ID3D12Resource> TempBuffer1;
	ComPtr<ID3D12Resource> TempBuffer2;

	ComPtr<ID3D12Resource> VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

	ComPtr<ID3D12Resource> IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;

	void CommandsBegin(ID3D12PipelineState* PipelineState = nullptr);
	void CommandsEnd();
public:
	d3d_app(HWND Window, u32 ClientWidth, u32 ClientHeight);

	void OnInit(mesh& Mesh);
	void BeginRender();
	void Draw(mesh& Mesh);
	void DrawIndexed(mesh& Mesh);
	void EndRender();

	void FenceSignal();
	void FenceWait();
	void Flush();
};

#define RENDERER_DIRECTX_12_H_
#endif
