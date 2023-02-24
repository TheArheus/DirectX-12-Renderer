#ifndef DIRECTX_UTILITIES_H_

void GetDevice(ID3D12Device6** DeviceResult, bool HighPerformance = true)
{
	ComPtr<IDXGIFactory6> Factory;
	*DeviceResult = nullptr;
	CreateDXGIFactory1(IID_PPV_ARGS(&Factory));

	UINT i = 0;
	ComPtr<IDXGIAdapter1> pAdapter;
	for (u32 AdapterIndex = 0;
			Factory->EnumAdapterByGpuPreference(AdapterIndex, 
				(HighPerformance ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED), 
				 IID_PPV_ARGS(&pAdapter)) != DXGI_ERROR_NOT_FOUND;
		++AdapterIndex)
	{
		DXGI_ADAPTER_DESC1 Desc;
		pAdapter->GetDesc1(&Desc);

		if(SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device6), nullptr)))
		{
			break;
		}
	}

	D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(DeviceResult));
}

u32 GetFormatSize(DXGI_FORMAT Format)
{
	switch(Format)
	{
		case DXGI_FORMAT_UNKNOWN:
			{
				return 0;
			}
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			{
				return 16;
			}
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			{
				return 12;
			}
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
			{
				return 8;
			}
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			{
				return 4;
			}
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			{
				return 2;
			}
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_R1_UNORM:
			{
				return 1;
			}
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
			{
				return 4;
			}
#if 0
		case DXGI_FORMAT_BC1_TYPELESS:
			{
				return ;
			}
		case DXGI_FORMAT_BC1_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			{
				return ;
			}
		case DXGI_FORMAT_BC2_TYPELESS:
			{
				return ;
			}
		case DXGI_FORMAT_BC2_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			{
				return ;
			}
		case DXGI_FORMAT_BC3_TYPELESS:
			{
				return ;
			}
		case DXGI_FORMAT_BC3_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			{
				return ;
			}
		case DXGI_FORMAT_BC4_TYPELESS:
			{
				return ;
			}
		case DXGI_FORMAT_BC4_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_BC4_SNORM:
			{
				return ;
			}
		case DXGI_FORMAT_BC5_TYPELESS:
			{
				return ;
			}
		case DXGI_FORMAT_BC5_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_BC5_SNORM:
			{
				return ;
			}
		case DXGI_FORMAT_B5G6R5_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_B5G5R5A1_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_B8G8R8X8_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			{
				return ;
			}
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			{
				return ;
			}
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
			{
				return ;
			}
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			{
				return ;
			}
		case DXGI_FORMAT_BC6H_TYPELESS:
			{
				return ;
			}
		case DXGI_FORMAT_BC6H_UF16:
			{
				return ;
			}
		case DXGI_FORMAT_BC6H_SF16:
			{
				return ;
			}
		case DXGI_FORMAT_BC7_TYPELESS:
			{
				return ;
			}
		case DXGI_FORMAT_BC7_UNORM:
			{
				return ;
			}
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			{
				return ;
			}
		case DXGI_FORMAT_AYUV:
			{
				return ;
			}
		case DXGI_FORMAT_Y410:
			{
				return ;
			}
		case DXGI_FORMAT_Y416:
			{
				return ;
			}
		case DXGI_FORMAT_NV12:
			{
				return ;
			}
		case DXGI_FORMAT_P010:
			{
				return ;
			}
		case DXGI_FORMAT_P016:
			{
				return ;
			}
		case DXGI_FORMAT_420_OPAQUE:
			{
				return ;
			}
		case DXGI_FORMAT_YUY2:
			{
				return ;
			}
		case DXGI_FORMAT_Y210:
			{
				return ;
			}
		case DXGI_FORMAT_Y216:
			{
				return ;
			}
		case DXGI_FORMAT_NV11:
			{
				return ;
			}
		case DXGI_FORMAT_AI44:
			{
				return 1;
			}
		case DXGI_FORMAT_IA44:
			{
				return 1;
			}
		case DXGI_FORMAT_P8:
			{
				return 1;
			}
		case DXGI_FORMAT_A8P8:
			{
				return 1;
			}
		case DXGI_FORMAT_B4G4R4A4_UNORM:
			{
				return 2;
			}
		case DXGI_FORMAT_P208:
			{
				return 1;
			}
		case DXGI_FORMAT_V208:
			{
				return 1;
			}
		case DXGI_FORMAT_V408:
			{
				return 1;
			}
		case DXGI_FORMAT_FORCE_UINT:
			{
				return 4;
			}
#endif
	}

	return 1;
}

#define DIRECTX_UTILITIES_H_
#endif
