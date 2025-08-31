
internal void VertexFormatsInit(VertexFormats *formats)
{
	AddVertexBinding(&formats->vec3, sizeof(v3),
			 VK_VERTEX_INPUT_RATE_VERTEX);
	{
		AddVertexAttribute(&formats->vec3, VK_FORMAT_R32G32B32_SFLOAT,
				   0);
	}

	AddVertexBinding(&formats->model, sizeof(ModelVertex),
			 VK_VERTEX_INPUT_RATE_VERTEX);
	{
		AddVertexAttribute(&formats->model, VK_FORMAT_R32G32B32_SFLOAT,
				   offsetof(ModelVertex, position));
		AddVertexAttribute(&formats->model, VK_FORMAT_R32G32_SFLOAT,
				   offsetof(ModelVertex, texcoord));
		AddVertexAttribute(&formats->model, VK_FORMAT_R32G32B32_SFLOAT,
				   offsetof(ModelVertex, colour));
		AddVertexAttribute(&formats->model, VK_FORMAT_R32G32B32_SFLOAT,
				   offsetof(ModelVertex, normal));
		AddVertexAttribute(&formats->model, VK_FORMAT_R32G32B32_SFLOAT,
				   offsetof(ModelVertex, tangent));
		AddVertexAttribute(&formats->model, VK_FORMAT_R32G32B32_SFLOAT,
				   offsetof(ModelVertex, bitangent));
	}
}
