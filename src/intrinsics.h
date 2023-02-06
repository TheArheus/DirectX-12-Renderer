#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <fstream>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <type_traits>

#include "directx/d3dx12.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <D3DCompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <windows.h>

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

#define KB(val)  (val * 1000)
#define MB(val)  (KB(val) * 1000)
#define GB(val)  (MB(val) * 1000)

#define KiB(val) (val * 1024)
#define MiB(val) (KiB(val) * 1024)
#define GiB(val) (GiB(val) * 1024)

template<typename T> struct type_name_res;

#include "math.h"
#include "mesh.h"
#include "renderer_directx12.h"
#include "win32_window.h"

