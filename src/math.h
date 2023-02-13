
template<typename T>
union v2
{
	struct
	{
		T x, y;
	};
	T E[2];

	v2() = default;

	v2(T V) : x(V), y(V) {};
	v2(T _x, T _y) : x(_x), y(_y){};

	template<typename U>
	v2(const v2<U>& V){ x = V.x; y = V.y; };

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
};

template<typename T>
union v3
{
	struct
	{
		T x, y, z;
	};
	T E[3];

	v3() = default;

	v3(T V) : x(V), y(V), z(V) {};
	v3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {};

	template<typename U>
	v3(const v2<U>& V){ x = V.x; y = V.y; z = 0; };
	template<typename U>
	v3(const v3<U>& V){ x = V.x; y = V.y; z = V.z; };

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
};

template<typename T>
union v4
{
	struct
	{
		T x, y, z, w;
	};
	T E[4];

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
};

using vech2 = v2<u16>;
using vech3 = v3<u16>;
using vech4 = v4<u16>;

using vec2 = v2<r32>;
using vec3 = v3<r32>;
using vec4 = v4<r32>;

using veci2 = v2<s32>;
using veci3 = v3<s32>;
using veci4 = v4<s32>;

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
	struct hash<v2<u16>>
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
	struct hash<v4<u16>>
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

u64 AlignUp(u64 Size, u64 Align = 4)
{
	return (Size + (Align - 1)) & ~(Align - 1);
}

u64 AlignDown(u64 Size, u64 Align = 4)
{
	return Size & ~(Align - 1);
}

