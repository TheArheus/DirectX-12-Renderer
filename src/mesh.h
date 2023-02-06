#ifndef MESH_H_
#define MESH_H_

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
private:
};
}

struct mesh
{
	std::vector<vertex> Vertices;
	std::vector<u32> VertexIndices;

	void Load(std::string Path);
	void BuildTriangles();
};


#endif
