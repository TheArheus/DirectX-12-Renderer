
union v2
{
	struct
	{
		r32 x, y;
	};
	r32 E[2];

	r32& operator[](int Idx)
	{
		return E[Idx];
	}

	bool operator==(const v2& rhs) const
	{
		return (this->x == rhs.x) && (this->y == rhs.y);
	}
};

union v3
{
	struct
	{
		r32 x, y, z;
	};
	r32 E[3];

	r32& operator[](int Idx)
	{
		return E[Idx];
	}

	bool operator==(const v3& rhs) const
	{
		return (this->x == rhs.x) && (this->y == rhs.y) && (this->z && rhs.z);
	}
};

union v4
{
	struct
	{
		r32 x, y, z, w;
	};
	r32 E[4];

	r32& operator[](int Idx)
	{
		return E[Idx];
	}

	bool operator==(const v4& rhs) const
	{
		return (this->x == rhs.x) && (this->y == rhs.y) && (this->z && rhs.z) && (this->w && rhs.w);
	}
};

namespace std
{
	// NOTE: got from stack overflow question
	void hash_combine(size_t& seed, size_t hash)
	{
		hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash;
	}	

	template<>
	struct hash<v2>
	{
		size_t operator()(const v2& v) const
		{
			size_t Result = 0;
			hash_combine(Result, hash<float>()(v.x));
			hash_combine(Result, hash<float>()(v.y));
			return Result;
		}
	};

	template<>
	struct hash<v3>
	{
		size_t operator()(const v3& v) const
		{
			size_t Result = 0;
			hash_combine(Result, hash<float>()(v.x));
			hash_combine(Result, hash<float>()(v.y));
			hash_combine(Result, hash<float>()(v.z));
			return Result;
		}
	};

	template<>
	struct hash<v4>
	{
		size_t operator()(const v4& v) const
		{
			size_t Result = 0;
			hash_combine(Result, hash<float>()(v.x));
			hash_combine(Result, hash<float>()(v.y));
			hash_combine(Result, hash<float>()(v.z));
			hash_combine(Result, hash<float>()(v.w));
			return Result;
		}
	};
}

