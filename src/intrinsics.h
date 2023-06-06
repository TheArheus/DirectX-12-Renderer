#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <fstream>
#include <string_view>
#include <vector>
#include <map>
#include <array>
#include <bitset>
#include <unordered_map>
#include <initializer_list>
#include <type_traits>

#include "directx/d3dx12.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <D3DCompiler.h>
#include <DirectXMath.h>
#include <dxgidebug.h>
#include <wrl.h>
#include <windows.h>

using namespace Microsoft::WRL;
using namespace DirectX;

// NOTE: unsigned 
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// NOTE: signed
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
                 
typedef float    r32;
typedef double   r64;

typedef uint32_t b32;
typedef uint64_t b64;

constexpr size_t KB(size_t val) { return val * 1000; };
constexpr size_t MB(size_t val) { return KB(val) * 1000; };
constexpr size_t GB(size_t val) { return MB(val) * 1000; };

constexpr size_t KiB(size_t val) { return val * 1024; };
constexpr size_t MiB(size_t val) { return KiB(val) * 1024; };
constexpr size_t GiB(size_t val) { return MiB(val) * 1024; };

template<typename T> struct type_name;

u32 GetImageMipLevels(u32 Width, u32 Height)
{
	u32 Result = 1;

	while(Width > 1 || Height > 1)
	{
		Result++;
		Width  >>= 1;
		Height >>= 1;
	}

	return Result;
}

u32 PreviousPowerOfTwo(u32 x)
{
	x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x - (x >> 1);
}

#include "math.h"
#include "mesh.h"
#include "directx_utilities.hpp"
#include "resources.hpp"
#include "indirect_command_signature.hpp"
#include "shader_input_signature.hpp"
#include "renderer_directx12.h"
#include "pipeline_contexts.hpp"
#include "win32_window.h"

