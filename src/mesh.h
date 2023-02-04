#ifndef MESH_H_
#define MESH_H_


struct vertex
{
	v3 Position;
	v2 TextureCoord;
	v3 Normal;

	bool operator==(const vertex& rhs) const
	{
		bool Res1 = this->Position == rhs.Position;
		bool Res2 = this->TextureCoord == rhs.TextureCoord;
		bool Res3 = this->Normal == rhs.Normal;

		return Res1 && Res2 && Res3;
	}
};

namespace std
{
template<>
struct hash<vertex>
{
	size_t operator()(const vertex& VertData) const
	{
		size_t res = 0;
		std::hash_combine(res, hash<v3>()(VertData.Position));
		std::hash_combine(res, hash<v2>()(VertData.TextureCoord));
		std::hash_combine(res, hash<v3>()(VertData.Normal));
		return res;
	}
private:
};
}

struct mesh
{
	std::vector<vertex> Vertices;
	std::vector<u32> VertexIndices;

	std::vector<v3> Coords;
	std::vector<v2> TextCoords;
	std::vector<v3> Normals;

	std::vector<u32> CoordIndices;
	std::vector<u32> TextCoordIndices;
	std::vector<u32> NormalIndices;

	void Load(std::string Path);
	void BuildTriangles();
};


#endif
