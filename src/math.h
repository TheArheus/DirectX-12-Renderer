
#pragma once

#include <immintrin.h>

template<typename T>
constexpr T Pi = T(3.1415926535897932384626433832795028841971693993751058209749445923078164062);

inline uint16_t
EncodeHalf(float x)
{
    uint32_t v = *(uint32_t*)&x;

    uint32_t s = (v & 0x8000'0000) >> 31;
    uint32_t e = (v & 0x7f80'0000) >> 23;
    uint32_t m = (v & 0x007f'ffff);

    if(e >= 255)	 return (s << 15) | 0x7c00;
    if(e > (127+15)) return (s << 15) | 0x7c00;
    if(e < (127-14)) return (s << 15) | 0x0 | (m >> 13);

    e = e - 127 + 15;
    m = m / float(1 << 23) * float(1 << 10);

    return (s << 15) | (e << 10) | (m);
}

inline float
DecodeHalf(uint16_t Val)
{
    float Res = 0;

    bool Sign =  Val & 0x8000;
    uint8_t   Exp  = (Val & 0x7C00) >> 10;
    uint16_t  Mant =  Val & 0x03FF;

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
        requires std::is_same_v<typename vec_t::type, uint16_t>
    operator vec_c<float>()
    {
        return vec_c<float>(DecodeHalf(E[a]), DecodeHalf(E[b]));
    }
    template<class vec_c>
    operator vec_c()
    {
        return vec_c(E[a], E[b]);
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
        requires std::is_same_v<typename vec_t::type, uint16_t>
    operator vec_c<float>()
    {
        return vec_c<float>(DecodeHalf(E[a]), DecodeHalf(E[b]), DecodeHalf(E[c]));
    }
    template<class vec_c>
    operator vec_c()
    {
        return vec_c(E[a], E[b], E[c]);
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
        requires std::is_same_v<typename vec_t::type, uint16_t>
    operator vec_c<float>()
    {
        return vec_c<float>(DecodeHalf(E[a]), DecodeHalf(E[b]), DecodeHalf(E[c]), DecodeHalf(E[d]));
    }
    template<class vec_c>
    operator vec_c()
    {
        return vec_c(E[a], E[b], E[c], E[d]);
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
        swizzle_2d<v2<T>, 0, 0> xx;
        swizzle_2d<v2<T>, 1, 1> yy;
        swizzle_2d<v2<T>, 0, 1> xy;
    };

    v2() = default;

    v2(T V) : x(V), y(V) {};
    v2(T _x, T _y) : x(_x), y(_y){};

    T& operator[](uint32_t Idx)
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

    v2 operator+(const float& rhs)
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

    v2& operator+=(const float& rhs)
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

    v2 operator-(const float& rhs)
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

    v2& operator-=(const float& rhs)
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

    v2 operator*(const float& rhs)
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

    v2& operator*=(const float& rhs)
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

    v2 operator/(const float& rhs)
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

    v2& operator/=(const float& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    bool operator==(const v2& rhs) const
    {
        return (this->x == rhs.x) && (this->y == rhs.y);
    }

    float Dot(const v2& rhs)
    {
        return this->x * rhs.x + this->y * rhs.y;
    }

    float LengthSq()
    {
        return Dot(*this);
    }

    float Length()
    {
        return sqrtf(LengthSq());
    }

    v2 Normalize()
    {
        *this = *this / Length();
        return *this;
    }
};

struct vech2 : v2<uint16_t>
{
    using v2::v2;
    vech2(const v2<float>& V)
    {
        x = EncodeHalf(V.x);
        y = EncodeHalf(V.y);
    }
};

#if 0
struct vec2 : v2<float>
{
    using v2::v2;
    bool operator==(const vec2& rhs) const
    {
        bool r0 = (fabs(this->x - rhs.x) <= std::numeric_limits<float>::epsilon() * std::max(fabs(this->x), fabs(rhs.x)));
        bool r1 = (fabs(this->y - rhs.y) <= std::numeric_limits<float>::epsilon() * std::max(fabs(this->y), fabs(rhs.y)));
        return r0 && r1;
    }
};
#endif

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
        swizzle_2d<v2<T>, 0, 1>    xy;
        swizzle_2d<v2<T>, 0, 0>    xx;
        swizzle_2d<v2<T>, 1, 1>    yy;
        swizzle_3d<v3<T>, 0, 0, 0> xxx;
        swizzle_3d<v3<T>, 1, 1, 1> yyy;
        swizzle_3d<v3<T>, 2, 2, 2> zzz;
        swizzle_3d<v3<T>, 0, 1, 2> xyz;
    };

    v3() = default;

    v3(T V) : x(V), y(V), z(V) {};
    v3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {};

    T& operator[](uint32_t Idx)
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

    v3 operator+(const float& rhs)
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

    v3& operator+=(const float& rhs)
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

    v3 operator-(const float& rhs)
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

    v3& operator-=(const float& rhs)
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

    v3 operator*(const float& rhs)
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

    v3& operator*=(const float& rhs)
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

    v3 operator/(const float& rhs)
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

    v3& operator/=(const float& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    bool operator==(const v3& rhs) const
    {
        return (this->x == rhs.x) && (this->y == rhs.y) && (this->z == rhs.z);
    }

    float Dot(const v3& rhs)
    {
        return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
    }

    float LengthSq()
    {
        return Dot(*this);
    }

    float Length()
    {
        return sqrtf(LengthSq());
    }

    v3 Normalize()
    {
        *this = *this / Length();
        return *this;
    }
};

struct vech3 : v3<uint16_t>
{
    using v3::v3;
    vech3(const v3<float>& V)
    {
        x = EncodeHalf(V.x);
        y = EncodeHalf(V.y);
        z = EncodeHalf(V.z);
    }
    vech3(const v3<uint16_t>& V)
    {
        x = V.x;
        y = V.y;
        z = V.z;
    }
    vech3(uint16_t _x, uint16_t _y, uint16_t _z)
    {
        x = _x;
        y = _y;
        z = _z;
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
        swizzle_2d<v2<T>, 0, 1>       xy;
        swizzle_2d<v2<T>, 0, 0>       xx;
        swizzle_2d<v2<T>, 1, 1>       yy;
        swizzle_3d<v3<T>, 0, 0, 0>    xxx;
        swizzle_3d<v3<T>, 1, 1, 1>    yyy;
        swizzle_3d<v3<T>, 2, 2, 2>    zzz;
        swizzle_3d<v3<T>, 0, 1, 2>    xyz;
        swizzle_4d<v4<T>, 0, 1, 2, 3> xyzw;
        swizzle_4d<v4<T>, 0, 0, 0, 0> xxxx;
        swizzle_4d<v4<T>, 1, 1, 1, 1> yyyy;
        swizzle_4d<v4<T>, 2, 2, 2, 2> zzzz;
        swizzle_4d<v4<T>, 3, 3, 3, 3> wwww;
    };

    v4() = default;

    v4(T V) : x(V), y(V), z(V), w(V) {};
    v4(v2<T> V) : x(V.x), y(V.y), z(0), w(0) {};
    v4(v3<T> V) : x(V.x), y(V.y), z(V.z), w(0) {};
    v4(v3<T> V, T _w) : x(V.x), y(V.y), z(V.z), w(_w) {};
    //v4(const v4<T>& V) : x(V.x), y(V.y), z(V.z), w(V.w) {};
    template<typename U>
    v4(U _x, U _y, U _z, U _w) : x(_x), y(_y), z(_z), w(_w) {};
    v4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {};

    T& operator[](uint32_t Idx)
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

    v4 operator+(const float& rhs)
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

    v4& operator+=(const float& rhs)
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

    v4 operator-(const float& rhs)
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

    v4& operator-=(const float& rhs)
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

    v4 operator*(const float& rhs)
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

    v4& operator*=(const float& rhs)
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

    v4 operator/(const float& rhs)
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

    v4& operator/=(const float& rhs)
    {
        *this = *this / rhs;
        return *this;
    }

    v4<T> operator=(const v4<T>& rhs)
    {
        this->x = rhs.x;
        this->y = rhs.y;
        this->z = rhs.z;
        this->w = rhs.w;
        return *this;
    }

    template<typename U>
    v4 operator=(const v4<U>& rhs)
    {
        this->x = rhs.x;
        this->y = rhs.y;
        this->z = rhs.z;
        this->w = rhs.w;
        return *this;
    }


    bool operator==(const v4& rhs) const
    {
        return (this->x == rhs.x) && (this->y == rhs.y) && (this->z == rhs.z) && (this->w == rhs.w);
    }

    float Dot(const v4& rhs)
    {
        return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z + this->w * rhs.w;
    }

    float LengthSq()
    {
        return Dot(*this);
    }

    float Length()
    {
        return sqrtf(LengthSq());
    }

    v4 Normalize()
    {
        *this = *this / Length();
        return *this;
    }
};

struct vech4 : v4<uint16_t>
{
    using v4::v4;
    vech4(const v4<float>& V)
    {
        x = EncodeHalf(V.x);
        y = EncodeHalf(V.y);
        z = EncodeHalf(V.z);
        w = EncodeHalf(V.w);
    }
    vech4(const v3<float>& V, float W)
    {
        x = EncodeHalf(V.x);
        y = EncodeHalf(V.y);
        z = EncodeHalf(V.z);
        w = EncodeHalf(W);
    }
    vech4(float X, float Y, float Z, float W)
    {
        x = EncodeHalf(X);
        y = EncodeHalf(Y);
        z = EncodeHalf(Z);
        w = EncodeHalf(W);
    }
};

template<typename T>
inline v4<T>
operator*(const v4<T>& lhs, const v4<T>& rhs)
{
    v4<T> Result = {};
    Result.x = lhs.x * rhs.x;
    Result.y = lhs.y * rhs.y;
    Result.z = lhs.z * rhs.z;
    Result.w = lhs.w * rhs.w;
    return Result;
}

template<typename T>
inline v4<T>
operator*(const v4<T>& lhs, const float& rhs)
{
    v4<T> Result = {};
    Result.x = lhs.x * rhs;
    Result.y = lhs.y * rhs;
    Result.z = lhs.z * rhs;
    Result.w = lhs.w * rhs;
    return Result;
}

template<typename T>
inline v4<T>
operator*=(v4<T> lhs, const v4<T>& rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

template<typename T>
inline v4<T>
operator*=(v4<T> lhs, const float& rhs)
{
    lhs = lhs * rhs;
    return lhs;
}

using vec2 = v2<float>;
using vec3 = v3<float>;
using vec4 = v4<float>;

using veci2 = v2<int32_t>;
using veci3 = v3<int32_t>;
using veci4 = v4<int32_t>;

inline float
Lerp(float a, float t, float b)
{
    return (1.0f - t) * a + t * b;
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

template<typename T>
inline float
Cross(v2<T> A, v2<T> B)
{
    float Result = A.x * B.y - A.y * B.x;
    return Result;
}

template<typename T>
inline v3<T>
Cross(v3<T> A, v3<T> B)
{
    v3<T> Result = {};

    Result.x = (A.y * B.z - A.z * B.y);
    Result.y = (A.z * B.x - A.x * B.z);
    Result.z = (A.x * B.y - A.y * B.x);

    return Result;
}

union mat3
{
    struct
    {
        float E11, E12, E13;
        float E21, E22, E23;
        float E31, E32, E33;
    };
    struct
    {
        vec3 Line0;
        vec3 Line1;
        vec3 Line2;
    };
    vec3 Lines[3];
    float E[3][3];
    float V[9];
};

inline vec3
operator*(const mat3 lhs, const vec3 rhs)
{
    vec3 Result = {};

    Result.x = lhs.E11 * rhs.x + lhs.E12 * rhs.y + lhs.E13 * rhs.z;
    Result.y = lhs.E21 * rhs.x + lhs.E22 * rhs.y + lhs.E23 * rhs.z;
    Result.z = lhs.E31 * rhs.x + lhs.E32 * rhs.y + lhs.E33 * rhs.z;

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

    mat4 operator=(const mat4& rhs)
    {
        this->Line0 = rhs.Line0;
        this->Line1 = rhs.Line1;
        this->Line2 = rhs.Line2;
        this->Line3 = rhs.Line3;
        return *this;
    }

    mat3 GetMat3()
    {
        mat3 Result =
        {
            E11, E12, E13,
            E21, E22, E23,
            E31, E32, E33,
        };
        return Result;
    }
};

inline vec4
operator*(const mat4 lhs, const vec4 rhs)
{
    vec4 Result = {};

    Result.x = lhs.E11 * rhs.x + lhs.E12 * rhs.y + lhs.E13 * rhs.z + lhs.E14 * rhs.w;
    Result.y = lhs.E21 * rhs.x + lhs.E22 * rhs.y + lhs.E23 * rhs.z + lhs.E24 * rhs.w;
    Result.z = lhs.E31 * rhs.x + lhs.E32 * rhs.y + lhs.E33 * rhs.z + lhs.E34 * rhs.w;
    Result.w = lhs.E41 * rhs.x + lhs.E42 * rhs.y + lhs.E43 * rhs.z + lhs.E44 * rhs.w;

    return Result;
}

inline mat4
Identity()
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

inline mat4
Scale(float V)
{
    mat4 Result = Identity();
    Result.E[0][0] = V;
    Result.E[1][1] = V;
    Result.E[2][2] = V;
    return Result;
}

inline mat4
Scale(vec3 V)
{
    mat4 Result = Identity();
    Result.E[0][0] = V.x;
    Result.E[1][1] = V.y;
    Result.E[2][2] = V.z;
    return Result;
}

inline mat4
RotateX(float A)
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

inline mat4
RotateY(float A)
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

inline mat4
RotateZ(float A)
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

inline mat4
Translate(float V)
{
    mat4 Result = Identity();
    Result.E[0][3] = V;
    Result.E[1][3] = V;
    Result.E[2][3] = V;
    return Result;
}

inline mat4
Translate(vec3 V)
{
    mat4 Result = Identity();
    Result.E[0][3] = V.x;
    Result.E[1][3] = V.y;
    Result.E[2][3] = V.z;
    return Result;
}

inline mat4
operator*(const mat4& lhs, const mat4& rhs)
{
    mat4 res = {};

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
        res.I[idx] = _mm_fmadd_ps(rhs.I[0], v0, res.I[idx]);
        res.I[idx] = _mm_fmadd_ps(rhs.I[1], v1, res.I[idx]);
        res.I[idx] = _mm_fmadd_ps(rhs.I[2], v2, res.I[idx]);
        res.I[idx] = _mm_fmadd_ps(rhs.I[3], v3, res.I[idx]);
    }

    return res;
}

struct quat
{
    union
	{
		struct 
		{
			vec3 q;
			float w;
		};
		vec4 v;
	};

	quat() = default;

	quat(float x_, float y_, float z_, float w_){w = w_; q.x = x_; q.y = y_; q.z = z_;};
	quat(vec3 q_, float w_){w = w_; q = q_;};
	quat(v4<float> v_){v = v_;};
	quat(float angle, vec3 axis){w = cos(angle / 2), q = axis * vec3(sin(angle/2));};

	quat(const quat& Other){ v = Other.v; };
	quat& operator=(const quat& Other)
    {
		v = Other.v;
        return *this; 
    };

	quat Inverse()
	{
		return quat(-q.x, -q.y, -q.z, w);
	}

	quat Normalize()
	{
		v = v.Normalize();
		return *this;
	};

	mat4 ToRotateMatrix()
	{
		mat4 Result = 
		{
			v.x*v.x - v.y*v.y - v.z*v.z + v.w*v.w, 2*v.x*v.y - 2*v.z*v.w, 2*v.y*v.w + 2*v.x*v.z, 0,
			2*v.z*v.w + 2*v.x*v.y, -v.x*v.x + v.y*v.y - v.z*v.z + v.w*v.w, 2*v.x*v.w - 2*v.x*v.w, 0,
			2*v.x*v.z - 2*v.y*v.w, 2*v.x*v.w + 2*v.y*v.z, -v.x*v.x - v.y*v.y + v.z*v.z + v.w*v.w, 0,
			0, 0, 0, 1
		};
		return Result;
	}
};

quat operator+(quat lhs, quat rhs)
{
	return quat(lhs.q.x + rhs.q.x, lhs.q.y + rhs.q.y, lhs.q.z + rhs.q.z, lhs.w + rhs.w);
}

quat operator*(quat lhs, quat rhs)
{
	return quat(lhs.q * rhs.w + rhs.q * lhs.w + Cross(lhs.q, rhs.q), -lhs.q.Dot(rhs.q) + lhs.w * rhs.w);
}

inline mat4
Perspective(float Fov, float Width, float Height, float NearZ, float FarZ)
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

inline mat4
PerspectiveInfFarZ(float Fov, float Width, float Height, float NearZ)
{
    float a = Height / Width;
    float f = 1.0f / tanf(Fov / 2.0f);
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
    uint32_t  Pad0;
    vec3 Norm;
    uint32_t  Pad1;
};

inline void
GeneratePlanes(plane* Planes, mat4 Proj, float NearZ, float FarZ = 0)
{
    FarZ = (FarZ < NearZ) ? NearZ : FarZ;
    Planes[0].Pos = vec3(0);
    Planes[0].Norm = vec3(Proj.E41 + Proj.E11, Proj.E42 + Proj.E12, Proj.E43 + Proj.E13).Normalize();

    Planes[1].Pos = vec3(0);
    Planes[1].Norm = vec3(Proj.E41 - Proj.E11, Proj.E42 - Proj.E12, Proj.E43 - Proj.E13).Normalize();

    Planes[2].Pos = vec3(0);
    Planes[2].Norm = vec3(Proj.E41 + Proj.E21, Proj.E42 + Proj.E22, Proj.E43 + Proj.E23).Normalize();

    Planes[3].Pos = vec3(0);
    Planes[3].Norm = vec3(Proj.E41 - Proj.E21, Proj.E42 - Proj.E22, Proj.E43 - Proj.E23).Normalize();

    Planes[4].Pos = vec3(0, 0, NearZ);
    Planes[4].Norm = vec3(Proj.E41 + Proj.E31, Proj.E42 + Proj.E32, Proj.E43 + Proj.E33).Normalize();

    Planes[5].Pos = vec3(0, 0, FarZ);
    Planes[5].Norm = vec3(Proj.E41 - Proj.E31, Proj.E42 - Proj.E32, Proj.E43 - Proj.E33).Normalize();
}

inline mat4
LookAt(vec3 CamPos, vec3 Target, vec3 Up)
{
    vec3 z = (CamPos - Target).Normalize();
    vec3 x = Cross(Up, z).Normalize();
    vec3 y = Cross(z, x);

    mat4 Result =
    {
        x.x, x.y, x.z, -CamPos.Dot(x),
        y.x, y.y, y.z, -CamPos.Dot(y),
        z.x, z.y, z.z, -CamPos.Dot(z),
          0,   0,   0,             1,
    };
    return Result;
}

namespace std
{
    // NOTE: got from stack overflow question
    inline void
    hash_combine(size_t& seed, size_t hash)
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
            hash_combine(Result, hash<T>{}(v.x));
            hash_combine(Result, hash<T>{}(v.y));
            return Result;
        }
    };

    template<typename T>
    struct hash<v3<T>>
    {
        size_t operator()(const v3<T>& v) const
        {
            size_t Result = 0;
            hash_combine(Result, hash<T>{}(v.x));
            hash_combine(Result, hash<T>{}(v.y));
            hash_combine(Result, hash<T>{}(v.z));
            return Result;
        }
    };

    template<typename T>
    struct hash<v4<T>>
    {
        size_t operator()(const v4<T>& v) const
        {
            size_t Result = 0;
            hash_combine(Result, hash<T>{}(v.x));
            hash_combine(Result, hash<T>{}(v.y));
            hash_combine(Result, hash<T>{}(v.z));
            hash_combine(Result, hash<T>{}(v.w));
            return Result;
        }
    };

    template<>
    struct hash<vech2>
    {
        size_t operator()(const v2<uint16_t>& v) const
        {
            size_t ValueToHash = 0;
            size_t Result = 0;

            ValueToHash = (v.x << 16) | (v.y);
            hash_combine(Result, hash<uint32_t>()(ValueToHash));
            return Result;
        }
    };

    template<>
    struct hash<vech3>
    {
        size_t operator()(const v3<uint16_t>& v) const
        {
            size_t ValueToHash = 0;
            size_t Result = 0;

            ValueToHash = (v.x << 16) | (v.y);
            hash_combine(Result, hash<uint32_t>()(ValueToHash));
            ValueToHash = v.z;
            hash_combine(Result, hash<uint16_t>()(ValueToHash));
            return Result;
        }
    };

    template<>
    struct hash<vech4>
    {
        size_t operator()(const v4<uint16_t>& v) const
        {
            size_t ValueToHash = 0;
            size_t Result = 0;

            ValueToHash = (v.x << 16) | (v.y);
            hash_combine(Result, hash<uint32_t>()(ValueToHash));
            ValueToHash = (v.z << 16) | (v.w);
            hash_combine(Result, hash<uint32_t>()(ValueToHash));
            return Result;
        }
    };
}

inline uint64_t
AlignUp(uint64_t Size, uint64_t Align = 4)
{
    return (Size + (Align - 1)) & ~(Align - 1);
}

inline uint64_t
AlignDown(uint64_t Size, uint64_t Align = 4)
{
    return Size & ~(Align - 1);
}

