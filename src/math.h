
#pragma once

#include <immintrin.h>

template<typename T>
constexpr T Pi = T(3.1415926535897932384626433832795028841971693993751058209749445923078164062);

u16 EncodeHalf(float x)
{
	u32 v = *(u32*)&x;

	u32 s = (v & 0x8000'0000) >> 31;
	u32 e = (v & 0x7f80'0000) >> 23;
	u32 m = (v & 0x007f'ffff);

	if(e >= 255)	 return (s << 15) | 0x7c00;
	if(e > (127+15)) return (s << 15) | 0x7c00;
	if(e < (127-14)) return (s << 15) | 0x0 | (m >> 13);

	e = e - 127 + 15;
	m = m / float(1 << 23) * float(1 << 10);

	return (s << 15) | (e << 10) | (m);
}

float DecodeHalf(u16 Val)
{
	float Res = 0;

	bool Sign =  Val & 0x8000;
	u8   Exp  = (Val & 0x7C00) >> 10;
	u16  Mant =  Val & 0x03FF;

	Res = std::powf(-1, Sign) * (1 << (Exp - 15)) * (1 + float(Mant) / (1 << 10));

	return Res;
}

template<typename vec_t, unsigned int a, unsigned int b>
struct swizzle_2d
{
	typename vec_t::type E[2];
	swizzle_2d() = default;
	swizzle_2d(const vec_t& Vec)
	{
		E[a] = Vec.x;
		E[b] = Vec.y;
	}
	vec_t operator=(const vec_t& Vec)
	{
		return vec_t(E[a] = Vec.x, E[b] = Vec.y);
	}
	operator vec_t()
	{
		return vec_t(E[a], E[b]);
	}
	template<template<typename T> class vec_c>
		requires std::is_same_v<typename vec_t::type, u16>
	operator vec_c<r32>()
	{
		return vec_c<r32>(DecodeHalf(E[a]), DecodeHalf(E[b]));
	}
};

template<typename vec_t, unsigned int a, unsigned int b, unsigned int c>
struct swizzle_3d
{
	typename vec_t::type E[3];
	swizzle_3d() = default;
	swizzle_3d(const vec_t& Vec)
	{
		E[a] = Vec.x;
		E[b] = Vec.y;
		E[c] = Vec.z;
	}
	vec_t operator=(const vec_t& Vec)
	{
		return vec_t(E[a] = Vec.x, E[b] = Vec.y, E[c] = Vec.z);
	}
	operator vec_t()
	{
		return vec_t(E[a], E[b], E[c]);
	}
	template<template<typename T> class vec_c>
		requires std::is_same_v<typename vec_t::type, u16>
	operator vec_c<r32>()
	{
		return vec_c<r32>(DecodeHalf(E[a]), DecodeHalf(E[b]), DecodeHalf(E[c]));
	}
};

template<typename vec_t, unsigned int a, unsigned int b, unsigned int c, unsigned int d>
struct swizzle_4d
{
	typename vec_t::type E[4];
	swizzle_4d() = default;
	swizzle_4d(const vec_t& Vec)
	{
		E[a] = Vec.x;
		E[b] = Vec.y;
		E[c] = Vec.z;
		E[d] = Vec.w;
	}
	vec_t operator=(const vec_t& Vec)
	{
		return vec_t(E[a] = Vec.x, E[b] = Vec.y, E[c] = Vec.z, E[d] = Vec.w);
	}
	operator vec_t()
	{
		return vec_t(E[a], E[b], E[c], E[d]);
	}
	template<template<typename T> class vec_c>
		requires std::is_same_v<typename vec_t::type, u16>
	operator vec_c<r32>()
	{
		return vec_c<r32>(DecodeHalf(E[a]), DecodeHalf(E[b]), DecodeHalf(E[c]), DecodeHalf(E[d]));
	}
};

template<typename T>
struct v2
{
	using type = T;
	union
	{
		struct
		{
			T x, y;
		};
		T E[2];
		swizzle_2d<v2, 0, 0> xx;
		swizzle_2d<v2, 1, 1> yy;
		swizzle_2d<v2, 0, 1> xy;
	};

	v2() = default;

	v2(T V) : x(V), y(V) {};
	v2(T _x, T _y) : x(_x), y(_y){};

	T& operator[](u32 Idx)
	{
		return E[Idx];
	}

	v2 operator+(const v2& rhs)
	{
		v2 Result = {};
		Result.x = this->x + rhs.x;
		Result.y = this->y + rhs.y;
		return Result;
	}

	v2 operator+(const r32& rhs)
	{
		v2 Result = {};
		Result.x = this->x + rhs;
		Result.y = this->y + rhs;
		return Result;
	}

	v2& operator+=(const v2& rhs)
	{
		*this = *this + rhs;
		return *this;
	}

	v2& operator+=(const r32& rhs)
	{
		*this = *this + rhs;
		return *this;
	}

	v2 operator-(const v2& rhs)
	{
		v2 Result = {};
		Result.x = this->x - rhs.x;
		Result.y = this->y - rhs.y;
		return Result;
	}

	v2 operator-(const r32& rhs)
	{
		v2 Result = {};
		Result.x = this->x - rhs;
		Result.y = this->y - rhs;
		return Result;
	}

	v2& operator-=(const v2& rhs)
	{
		*this = *this - rhs;
		return *this;
	}

	v2& operator-=(const r32& rhs)
	{
		*this = *this - rhs;
		return *this;
	}

	v2 operator*(const v2& rhs)
	{
		v2 Result = {};
		Result.x = this->x * rhs.x;
		Result.y = this->y * rhs.y;
		return Result;
	}

	v2 operator*(const r32& rhs)
	{
		v2 Result = {};
		Result.x = this->x * rhs;
		Result.y = this->y * rhs;
		return Result;
	}

	v2& operator*=(const v2& rhs)
	{
		*this = *this * rhs;
		return *this;
	}

	v2& operator*=(const r32& rhs)
	{
		*this = *this * rhs;
		return *this;
	}

	v2 operator/(const v2& rhs)
	{
		v2 Result = {};
		Result.x = this->x / rhs.x;
		Result.y = this->y / rhs.y;
		return Result;
	}

	v2 operator/(const r32& rhs)
	{
		v2 Result = {};
		Result.x = this->x / rhs;
		Result.y = this->y / rhs;
		return Result;
	}

	v2& operator/=(const v2& rhs)
	{
		*this = *this / rhs;
		return *this;
	}

	v2& operator/=(const r32& rhs)
	{
		*this = *this / rhs;
		return *this;
	}

	bool operator==(const v2& rhs) const
	{
		return (this->x == rhs.x) && (this->y == rhs.y);
	}

	operator v2<u16>()
	{
		return v2<r32>(DecodeHalf(x), DecodeHalf(y));
	}

	r32 Dot(const v2& rhs)
	{
		return this->x * rhs.x + this->y * rhs.y;
	}

	r32 LengthSq()
	{
		return Dot(*this);
	}

	r32 Length()
	{
		return sqrtf(LengthSq());
	}

	void Normalize()
	{
		*this = *this / Length();
	}
};

struct vech2 : v2<u16>
{
	using v2::v2;
	vech2(const v2<r32>& V)
	{
		x = EncodeHalf(V.x);
		y = EncodeHalf(V.y);
	}
};

template<typename T>
struct v3
{
	using type = T;
	union
	{
		struct
		{
			T x, y, z;
		};
		struct
		{
			T r, g, b;
		};
		T E[3];
		swizzle_2d<v2<T>, 0, 1> xy;
		swizzle_2d<v2<T>, 0, 0> xx;
		swizzle_2d<v2<T>, 1, 1> yy;
		swizzle_3d<v3, 0, 0, 0> xxx;
		swizzle_3d<v3, 1, 1, 1> yyy;
		swizzle_3d<v3, 2, 2, 2> zzz;
		swizzle_3d<v3, 0, 1, 2> xyz;
	};

	v3() = default;

	v3(T V) : x(V), y(V), z(V) {};
	v3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {};

	T& operator[](u32 Idx)
	{
		return E[Idx];
	}

	v3 operator+(const v3& rhs)
	{
		v3 Result = {};
		Result.x = this->x + rhs.x;
		Result.y = this->y + rhs.y;
		Result.z = this->z + rhs.z;
		return Result;
	}

	v3 operator+(const r32& rhs)
	{
		v3 Result = {};
		Result.x = this->x + rhs;
		Result.y = this->y + rhs;
		Result.z = this->z + rhs;
		return Result;
	}

	v3& operator+=(const v3& rhs)
	{
		*this = *this + rhs;
		return *this;
	}

	v3& operator+=(const r32& rhs)
	{
		*this = *this + rhs;
		return *this;
	}

	v3 operator-(const v3& rhs)
	{
		v3 Result = {};
		Result.x = this->x - rhs.x;
		Result.y = this->y - rhs.y;
		Result.z = this->z - rhs.z;
		return Result;
	}

	v3 operator-(const r32& rhs)
	{
		v3 Result = {};
		Result.x = this->x - rhs;
		Result.y = this->y - rhs;
		Result.z = this->z - rhs;
		return Result;
	}

	v3& operator-=(const v3& rhs)
	{
		*this = *this - rhs;
		return *this;
	}

	v3& operator-=(const r32& rhs)
	{
		*this = *this - rhs;
		return *this;
	}

	v3 operator*(const v3& rhs)
	{
		v3 Result = {};
		Result.x = this->x * rhs.x;
		Result.y = this->y * rhs.y;
		Result.z = this->z * rhs.z;
		return Result;
	}

	v3 operator*(const r32& rhs)
	{
		v3 Result = {};
		Result.x = this->x * rhs;
		Result.y = this->y * rhs;
		Result.z = this->z * rhs;
		return Result;
	}

	v3& operator*=(const v3& rhs)
	{
		*this = *this * rhs;
		return *this;
	}

	v3& operator*=(const r32& rhs)
	{
		*this = *this * rhs;
		return *this;
	}

	v3 operator/(const v3& rhs)
	{
		v3 Result = {};
		Result.x = this->x / rhs.x;
		Result.y = this->y / rhs.y;
		Result.z = this->z / rhs.z;
		return Result;
	}

	v3 operator/(const r32& rhs)
	{
		v3 Result = {};
		Result.x = this->x / rhs;
		Result.y = this->y / rhs;
		Result.z = this->z / rhs;
		return Result;
	}

	v3& operator/=(const v3& rhs)
	{
		*this = *this / rhs;
		return *this;
	}

	v3& operator/=(const r32& rhs)
	{
		*this = *this / rhs;
		return *this;
	}

	bool operator==(const v3& rhs) const
	{
		return (this->x == rhs.x) && (this->y == rhs.y) && (this->z && rhs.z);
	}

	r32 Dot(const v3& rhs)
	{
		return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
	}

	r32 LengthSq()
	{
		return Dot(*this);
	}

	r32 Length()
	{
		return sqrtf(LengthSq());
	}

	void Normalize()
	{
		*this = *this / Length();
	}
};

struct vech3 : v3<u16>
{
	using v3::v3;
	vech3(const v3<r32>& V)
	{
		x = EncodeHalf(V.x);
		y = EncodeHalf(V.y);
		z = EncodeHalf(V.z);
	}
};

template<typename T>
struct v4
{
	using type = T;
	union
	{
		struct
		{
			T x, y, z, w;
		};
		struct
		{
			T r, g, b, a;
		};
		T E[4];
		swizzle_2d<v2<T>, 0, 1>    xy;
		swizzle_2d<v2<T>, 0, 0>    xx;
		swizzle_2d<v2<T>, 1, 1>    yy;
		swizzle_3d<v3<T>, 0, 0, 0> xxx;
		swizzle_3d<v3<T>, 1, 1, 1> yyy;
		swizzle_3d<v3<T>, 2, 2, 2> zzz;
		swizzle_3d<v3<T>, 0, 1, 2> xyz;
		swizzle_4d<v4, 0, 1, 2, 3> xyzw;
		swizzle_4d<v4, 0, 0, 0, 0> xxxx;
		swizzle_4d<v4, 1, 1, 1, 1> yyyy;
		swizzle_4d<v4, 2, 2, 2, 2> zzzz;
		swizzle_4d<v4, 3, 3, 3, 3> wwww;
	};

	v4() = default;

	v4(T V) : x(V), y(V), z(V), w(V) {};
	v4(v2<T> V) : x(V.x), y(V.y), z(0), w(0) {};
	v4(v3<T> V) : x(V.x), y(V.y), z(V.z), w(0) {};
	v4(v3<T> V, T _w) : x(V.x), y(V.y), z(V.z), w(_w) {};
	v4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {};

	T& operator[](u32 Idx)
	{
		return E[Idx];
	}

	v4 operator+(const v4& rhs)
	{
		v4 Result = {};
		Result.x = this->x + rhs.x;
		Result.y = this->y + rhs.y;
		Result.z = this->z + rhs.z;
		Result.w = this->w + rhs.w;
		return Result;
	}

	v4 operator+(const r32& rhs)
	{
		v4 Result = {};
		Result.x = this->x + rhs;
		Result.y = this->y + rhs;
		Result.z = this->z + rhs;
		Result.w = this->w + rhs;
		return Result;
	}

	v4& operator+=(const v4& rhs)
	{
		*this = *this + rhs;
		return *this;
	}

	v4& operator+=(const r32& rhs)
	{
		*this = *this + rhs;
		return *this;
	}

	v4 operator-(const v4& rhs)
	{
		v4 Result = {};
		Result.x = this->x - rhs.x;
		Result.y = this->y - rhs.y;
		Result.z = this->z - rhs.z;
		Result.w = this->w - rhs.w;
		return Result;
	}

	v4 operator-(const r32& rhs)
	{
		v4 Result = {};
		Result.x = this->x - rhs;
		Result.y = this->y - rhs;
		Result.z = this->z - rhs;
		Result.w = this->w - rhs;
		return Result;
	}

	v4& operator-=(const v4& rhs)
	{
		*this = *this - rhs;
		return *this;
	}

	v4& operator-=(const r32& rhs)
	{
		*this = *this - rhs;
		return *this;
	}

	v4 operator*(const v4& rhs)
	{
		v4 Result = {};
		Result.x = this->x * rhs.x;
		Result.y = this->y * rhs.y;
		Result.z = this->z * rhs.z;
		Result.w = this->w * rhs.w;
		return Result;
	}

	v4 operator*(const r32& rhs)
	{
		v4 Result = {};
		Result.x = this->x * rhs;
		Result.y = this->y * rhs;
		Result.z = this->z * rhs;
		Result.w = this->w * rhs;
		return Result;
	}

	v4& operator*=(const v4& rhs)
	{
		*this = *this * rhs;
		return *this;
	}

	v4& operator*=(const r32& rhs)
	{
		*this = *this * rhs;
		return *this;
	}

	v4 operator/(const v4& rhs)
	{
		v4 Result = {};
		Result.x = this->x / rhs.x;
		Result.y = this->y / rhs.y;
		Result.z = this->z / rhs.z;
		Result.w = this->w / rhs.w;
		return Result;
	}

	v4 operator/(const r32& rhs)
	{
		v4 Result = {};
		Result.x = this->x / rhs;
		Result.y = this->y / rhs;
		Result.z = this->z / rhs;
		Result.w = this->w / rhs;
		return Result;
	}

	v4& operator/=(const v4& rhs)
	{
		*this = *this / rhs;
		return *this;
	}

	v4& operator/=(const r32& rhs)
	{
		*this = *this / rhs;
		return *this;
	}

	bool operator==(const v4& rhs) const
	{
		return (this->x == rhs.x) && (this->y == rhs.y) && (this->z && rhs.z) && (this->w && rhs.w);
	}

	operator v4<u16>()
	{
		return v4<r32>(DecodeHalf(x), DecodeHalf(y), DecodeHalf(z), DecodeHalf(w));
	}

	r32 Dot(const v4& rhs)
	{
		return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z + this->w * rhs.w;
	}

	r32 LengthSq()
	{
		return Dot(*this);
	}

	r32 Length()
	{
		return sqrtf(LengthSq());
	}

	void Normalize()
	{
		*this = *this / Length();
	}
};

struct vech4 : v4<u16>
{
	using v4::v4;
	vech4(const v4<r32>& V)
	{
		x = EncodeHalf(V.x);
		y = EncodeHalf(V.y);
		z = EncodeHalf(V.z);
		w = EncodeHalf(V.w);
	}
	vech4(const v3<r32>& V, r32 W)
	{
		x = EncodeHalf(V.x);
		y = EncodeHalf(V.y);
		z = EncodeHalf(V.z);
		w = EncodeHalf(W);
	}
	vech4(r32 X, r32 Y, r32 Z, r32 W)
	{
		x = EncodeHalf(X);
		y = EncodeHalf(Y);
		z = EncodeHalf(Z);
		w = EncodeHalf(W);
	}
};

template<typename T>
v4<T> operator*(const v4<T>& lhs, const v4<T>& rhs)
{
	v4<T> Result = {};
	Result.x = lhs.x * rhs.x;
	Result.y = lhs.y * rhs.y;
	Result.z = lhs.z * rhs.z;
	Result.w = lhs.w * rhs.w;
	return Result;
}

template<typename T>
v4<T> operator*(const v4<T>& lhs, const r32& rhs)
{
	v4<T> Result = {};
	Result.x = lhs.x * rhs;
	Result.y = lhs.y * rhs;
	Result.z = lhs.z * rhs;
	Result.w = lhs.w * rhs;
	return Result;
}

template<typename T>
v4<T> operator*=(v4<T> lhs, const v4<T>& rhs)
{
	lhs = lhs * rhs;
	return lhs;
}

template<typename T>
v4<T> operator*=(v4<T> lhs, const r32& rhs)
{
	lhs = lhs * rhs;
	return lhs;
}

using vec2 = v2<r32>;
using vec3 = v3<r32>;
using vec4 = v4<r32>;

using veci2 = v2<s32>;
using veci3 = v3<s32>;
using veci4 = v4<s32>;


inline float 
Lerp(float a, float t, float b)
{
    return (1 - t) * a + t * b;
}

template<typename T>
inline v2<T> 
Lerp(v2<T> a, float t, v2<T> b)
{
    v2<T> Result = {};

    Result.x = Lerp(a.x, t, b.x);
    Result.y = Lerp(a.y, t, b.y);

    return Result;
}

template<typename T>
inline v3<T> 
Lerp(v3<T> a, float t, v3<T> b)
{
    v3<T> Result = {};

    Result.x = Lerp(a.x, t, b.x);
    Result.y = Lerp(a.y, t, b.y);
    Result.z = Lerp(a.z, t, b.z);

    return Result;
}

template<typename T>
inline v4<T> 
Lerp(v4<T> a, float t, v4<T> b)
{
    v4<T> Result = {};

    Result.x = Lerp(a.x, t, b.x);
    Result.y = Lerp(a.y, t, b.y);
    Result.z = Lerp(a.z, t, b.z);
    Result.w = Lerp(a.w, t, b.w);

    return Result;
}

union mat4
{
	struct
	{
		float E11, E12, E13, E14;
		float E21, E22, E23, E24;
		float E31, E32, E33, E34;
		float E41, E42, E43, E44;
	};
	struct
	{
		vec4 Line0;
		vec4 Line1;
		vec4 Line2;
		vec4 Line3;
	};
	vec4 Lines[4];
	float E[4][4];
	float V[16];
	__m128 I[4];
};

inline vec4 operator*(const mat4 lhs, const vec4 rhs)
{
    vec4 Result = {};

    Result.x = lhs.E11 * rhs.x + lhs.E12 * rhs.y + lhs.E13 * rhs.z + lhs.E14 * rhs.w;
    Result.y = lhs.E21 * rhs.x + lhs.E22 * rhs.y + lhs.E23 * rhs.z + lhs.E24 * rhs.w;
    Result.z = lhs.E31 * rhs.x + lhs.E32 * rhs.y + lhs.E33 * rhs.z + lhs.E34 * rhs.w;
    Result.w = lhs.E41 * rhs.x + lhs.E42 * rhs.y + lhs.E43 * rhs.z + lhs.E44 * rhs.w;

    return Result;
}

inline mat4 Identity()
{
	mat4 Result = 
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	return Result;
}

inline mat4 Scale(float V)
{
	mat4 Result = Identity();
	Result.E[0][0] = V;
	Result.E[1][1] = V;
	Result.E[2][2] = V;
	return Result;
}

inline mat4 RotateX(float A)
{
	float s = sinf(A);
	float c = cosf(A);
	mat4 Result = 
	{
		1, 0,  0, 0,
		0, c, -s, 0,
		0, s,  c, 0,
		0, 0,  0, 1,
	};

	return Result;
}

inline mat4 RotateY(float A)
{
	float s = sinf(A);
	float c = cosf(A);
	mat4 Result = 
	{
		 c, 0, s, 0,
		 0, 1, 0, 0,
		-s, 0, c, 0,
		 0, 0, 0, 1,
	};

	return Result;
}

inline mat4 RotateZ(float A)
{
	float s = sinf(A);
	float c = cosf(A);
	mat4 Result = 
	{
		c, -s, 0, 0,
		s,  c, 0, 0,
		0,  0, 1, 0,
		0,  0, 0, 1,
	};

	return Result;
}

inline mat4 Translate(float V)
{
	mat4 Result = Identity();
	Result.E[0][3] = V;
	Result.E[1][3] = V;
	Result.E[2][3] = V;
	return Result;
}

inline mat4 operator*(const mat4& lhs, const mat4& rhs)
{
    mat4 res = {};
	__m128 resl[4] = {};

	const float* Line0 = rhs.Line0.E;
	const float* Line1 = rhs.Line1.E;
	const float* Line2 = rhs.Line2.E;
	const float* Line3 = rhs.Line3.E;

	__m128 l0 = _mm_load_ps(Line0);
	__m128 l1 = _mm_load_ps(Line1);
	__m128 l2 = _mm_load_ps(Line2);
	__m128 l3 = _mm_load_ps(Line3);
	__m128 v0 = {};
    __m128 v1 = {};
    __m128 v2 = {};
    __m128 v3 = {};

    for(int idx = 0; idx < 4; ++idx)
    {
		v0 = _mm_set1_ps(lhs.V[0+idx*4]);
		v1 = _mm_set1_ps(lhs.V[1+idx*4]);
		v2 = _mm_set1_ps(lhs.V[2+idx*4]);
		v3 = _mm_set1_ps(lhs.V[3+idx*4]);
		resl[idx] = _mm_fmadd_ps(l0, v0, resl[idx]);
		resl[idx] = _mm_fmadd_ps(l1, v1, resl[idx]);
		resl[idx] = _mm_fmadd_ps(l2, v2, resl[idx]);
		resl[idx] = _mm_fmadd_ps(l3, v3, resl[idx]);
    }

	_mm_store_ps((float*)&res.Line0.E, resl[0]);
	_mm_store_ps((float*)&res.Line1.E, resl[1]);
	_mm_store_ps((float*)&res.Line2.E, resl[2]);
	_mm_store_ps((float*)&res.Line3.E, resl[3]);

    return res;
}

inline mat4 Perspective(float Fov, float Width, float Height, float NearZ, float FarZ)
{
	float a = Height / Width;
	float f = cosf(0.5f * Fov) / sinf(0.5f * Fov);
	float l = FarZ / (FarZ - NearZ);
	mat4 Result = 
	{
		f*a, 0,  0, 0,
		 0,  f,  0, 0,
		 0,  0,  l, -l * NearZ,
		 0,  0,  1, 0,
	};

	return Result;
}

inline mat4 PerspectiveInfFarZ(float Fov, float Width, float Height, float NearZ)
{
	float a = Height / Width;
	float f = 1.0 / tanf(Fov / 2.0);
	mat4 Result = 
	{
		f*a, 0,  0, 0,
		 0,  f,  0, 0,
		 0,  0,  0, NearZ,
		 0,  0, -1, 0,
	};

	return Result;
}

struct plane
{
	vec3 Pos;
	u32  Pad0;
	vec3 Norm;
	u32  Pad1;
};

void GeneratePlanes(plane* Planes, mat4 Proj, float NearZ, float FarZ = 0)
{
	FarZ = (FarZ < NearZ) ? NearZ : FarZ;
	Planes[0].Pos = vec3(0);
	Planes[0].Norm = vec3(Proj.E41 + Proj.E11, Proj.E42 + Proj.E12, Proj.E43 + Proj.E13);
	Planes[0].Norm /= Planes[0].Norm.Length();

	Planes[1].Pos = vec3(0);
	Planes[1].Norm = vec3(Proj.E41 - Proj.E11, Proj.E42 - Proj.E12, Proj.E43 - Proj.E13);
	Planes[1].Norm /= Planes[1].Norm.Length();

	Planes[2].Pos = vec3(0);
	Planes[2].Norm = vec3(Proj.E41 + Proj.E21, Proj.E42 + Proj.E22, Proj.E43 + Proj.E23);
	Planes[2].Norm /= Planes[2].Norm.Length();

	Planes[3].Pos = vec3(0);
	Planes[3].Norm = vec3(Proj.E41 - Proj.E21, Proj.E42 - Proj.E22, Proj.E43 - Proj.E23);
	Planes[3].Norm /= Planes[3].Norm.Length();

	Planes[4].Pos = vec3(0, 0, NearZ);
	Planes[4].Norm = vec3(Proj.E41 + Proj.E31, Proj.E42 + Proj.E32, Proj.E43 + Proj.E33);
	Planes[4].Norm /= Planes[4].Norm.Length();
	
	Planes[5].Pos = vec3(0, 0, FarZ);
	Planes[5].Norm = vec3(Proj.E41 - Proj.E31, Proj.E42 - Proj.E32, Proj.E43 - Proj.E33);
	Planes[5].Norm /= Planes[5].Norm.Length();
}

namespace std
{
	// NOTE: got from stack overflow question
	void hash_combine(size_t& seed, size_t hash)
	{
		hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash;
	}	

	template<typename T>
	struct hash<v2<T>>
	{
		size_t operator()(const v2<T>& v) const
		{
			size_t Result = 0;
			hash_combine(Result, hash<T>()(v.x));
			hash_combine(Result, hash<T>()(v.y));
			return Result;
		}
	};

	template<typename T>
	struct hash<v3<T>>
	{
		size_t operator()(const v3<T>& v) const
		{
			size_t Result = 0;
			hash_combine(Result, hash<T>()(v.x));
			hash_combine(Result, hash<T>()(v.y));
			hash_combine(Result, hash<T>()(v.z));
			return Result;
		}
	};

	template<typename T>
	struct hash<v4<T>>
	{
		size_t operator()(const v4<T>& v) const
		{
			size_t Result = 0;
			hash_combine(Result, hash<T>()(v.x));
			hash_combine(Result, hash<T>()(v.y));
			hash_combine(Result, hash<T>()(v.z));
			hash_combine(Result, hash<T>()(v.w));
			return Result;
		}
	};

	template<>
	struct hash<vech2>
	{
		size_t operator()(const v2<u16>& v) const
		{
			size_t ValueToHash = 0;
			size_t Result = 0;

			ValueToHash = (v.x << 16) | (v.y);
			hash_combine(Result, hash<u32>()(ValueToHash));
			return Result;
		}
	};

	template<>
	struct hash<vech3>
	{
		size_t operator()(const v3<u16>& v) const
		{
			size_t ValueToHash = 0;
			size_t Result = 0;

			ValueToHash = (v.x << 16) | (v.y);
			hash_combine(Result, hash<u32>()(ValueToHash));
			ValueToHash = v.z;
			hash_combine(Result, hash<u16>()(ValueToHash));
			return Result;
		}
	};

	template<>
	struct hash<vech4>
	{
		size_t operator()(const v4<u16>& v) const
		{
			size_t ValueToHash = 0;
			size_t Result = 0;

			ValueToHash = (v.x << 16) | (v.y);
			hash_combine(Result, hash<u32>()(ValueToHash));
			ValueToHash = (v.z << 16) | (v.w);
			hash_combine(Result, hash<u32>()(ValueToHash));
			return Result;
		}
	};
}

u64 AlignUp(u64 Size, u64 Align = 4)
{
	return (Size + (Align - 1)) & ~(Align - 1);
}

u64 AlignDown(u64 Size, u64 Align = 4)
{
	return Size & ~(Align - 1);
}

