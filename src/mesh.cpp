
void mesh::Load(std::string Path)
{
	std::ifstream File(Path);
	if(File.is_open())
	{
		File.seekg(0, std::ios::end);
		size_t FileSize = File.tellg();
		File.seekg(0, std::ios::beg);

		std::string Content;
		v3 Vertex3 = {};
		v2 Vertex2 = {};
		while (std::getline(File, Content))
		{
			char* Line = const_cast<char*>(Content.c_str());
			if (Content[0] == '#' || Content[0] == 'm' || Content[0] == 'o' || Content[0] == 'u' || Content[0] == 's') continue;
			if (Content.rfind("v ", 0) != std::string::npos)
			{
				sscanf(Content.c_str(), "v %f %f %f", &Vertex3[0], &Vertex3[1], &Vertex3[2]);
				Coords.push_back(Vertex3);
			}
			if (Content.rfind("vn", 0) != std::string::npos)
			{
				sscanf(Content.c_str(), "vn %f %f %f", &Vertex3[0], &Vertex3[1], &Vertex3[2]);
				Normals.push_back(Vertex3);
			}
			if (Content.rfind("vt", 0) != std::string::npos)
			{
				sscanf(Content.c_str(), "vt %f %f", &Vertex2[0], &Vertex2[1]);
				TextCoords.push_back(Vertex2);
			}
			if (Content[0] == 'f')
			{
				u32 Indices[3][3] = {};
				char* ToParse = const_cast<char*>(Content.c_str()) + 1;

				int Idx = 0;
				int Type = 0;
				while(*ToParse++)
				{
					if(*ToParse != '/')
					{
						Indices[Type][Idx] = atoi(ToParse);
					}
					
					while ((*ToParse != ' ') && (*ToParse != '/') && (*ToParse))
					{
						ToParse++;
					}

					if(*ToParse == '/')
					{
						Type++;
					}
					if(*ToParse == ' ')
					{
						Type = 0;
						Idx++;
					}
				}

				CoordIndices.push_back(Indices[0][0]-1);
				CoordIndices.push_back(Indices[0][1]-1);
				CoordIndices.push_back(Indices[0][2]-1);
				if (Indices[1][0] != 0) 
				{
					TextCoordIndices.push_back(Indices[1][0]-1);
					TextCoordIndices.push_back(Indices[1][1]-1);
					TextCoordIndices.push_back(Indices[1][2]-1);
				}
				if (Indices[2][0] != 0)
				{
					NormalIndices.push_back(Indices[2][0]-1);
					NormalIndices.push_back(Indices[2][1]-1);
					NormalIndices.push_back(Indices[2][2]-1);
				}
			}
		}
	}

	BuildTriangles();
}

void mesh::BuildTriangles()
{
	std::unordered_map<vertex, u32> UniqueVertices;
	u32 IndexCount = CoordIndices.size();
	std::vector<u32> RemapedIndices(IndexCount);

	for(u32 VertexIndex = 0;
		VertexIndex < IndexCount;
		++VertexIndex)
	{
		vertex Vert = {};

		v3 Pos = Coords[CoordIndices[VertexIndex]];
		Vert.Position = Pos;

		if(TextCoords.size() != 0)
		{
			v2 TextCoord = TextCoords[TextCoordIndices[VertexIndex]];
			Vert.TextureCoord = TextCoord;
		}

		if(NormalIndices.size() != 0)
		{
			v3 Norm = Normals[NormalIndices[VertexIndex]];
			Vert.Normal = Norm;
		} 

		if(UniqueVertices.count(Vert) == 0)
		{
			UniqueVertices[Vert] = static_cast<u32>(Vertices.size());
			Vertices.push_back(Vert);
		}

		RemapedIndices[VertexIndex] = UniqueVertices[Vert];
	}

	VertexIndices = std::move(RemapedIndices);
}

