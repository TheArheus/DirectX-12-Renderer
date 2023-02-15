#ifndef MESH_H_
#define MESH_H_

enum bounding_generation : u32
{
	generate_aabb = 0x1,
	generate_sphere = 0x2,
};

struct vertex
{
	vech4 Position;
	vec2  TextureCoord;
	u32   Normal;

	bool operator==(const vertex& rhs) const
	{
		bool Res1 = this->Position == rhs.Position;
		bool Res2 = this->TextureCoord == rhs.TextureCoord;
		bool Res3 = this->Normal == rhs.Normal;

		return Res1 && Res2 && Res3;
	}
};

struct meshlet
{
	u32 Vertices[64];
	u32 Indices[126*3];
	u32 VertexCount;
	u32 IndexCount;
};

namespace std
{
template<>
struct hash<vertex>
{
	size_t operator()(const vertex& VertData) const
	{
		size_t res = 0;
		std::hash_combine(res, hash<decltype(VertData.Position)>()(VertData.Position));
		std::hash_combine(res, hash<decltype(VertData.TextureCoord)>()(VertData.TextureCoord));
		std::hash_combine(res, hash<decltype(VertData.Normal)>()(VertData.Normal));
		return res;
	}
};
}

struct mesh
{
	struct offset
	{
		u32 VertexOffset;
		u32 VertexCount;

		u32 IndexOffset;
		u32 IndexCount;
	};

	struct sphere
	{
		vec3 Center;
		float Radius;
	};

	struct aabb
	{
		vec3 Min;
		vec3 Max;
	};

	mesh(const std::string& Path, u32 BoundingGeneration = 0);
	mesh(std::initializer_list<std::string> Paths, u32 BoundingGeneration = 0);
	void Load(const std::string& Path, u32 BoundingGeneration = 0);

	void GenerateMeshlets();

	void GenerateAxisAlignedBoundingBox(const std::vector<vec3>& Coords);
	void GenerateBoundingSphere();

	std::vector<vertex> Vertices;
	std::vector<u32> VertexIndices;
	std::vector<meshlet> Meshlets;
	std::vector<offset> Offsets;

	std::vector<sphere> BoundingSpheres;
	std::vector<aabb> AABBs;
};


#endif
