
internal void
VertexFormatsInit(VertexFormats *formats)
{
	AddVertexBinding(&formats->v3_format, sizeof(v3), VK_VERTEX_INPUT_RATE_VERTEX);
	{
		AddVertexAttribute(&formats->v3_format, VK_FORMAT_R32G32B32_SFLOAT, 0);
	}
	
	AddVertexBinding(&formats->model_format, sizeof(ModelVertex), VK_VERTEX_INPUT_RATE_VERTEX);
	{
		AddVertexAttribute(&formats->model_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, position));
		AddVertexAttribute(&formats->model_format, VK_FORMAT_R32G32_SFLOAT,    offsetof(ModelVertex, texcoord));
		AddVertexAttribute(&formats->model_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, colour));
		AddVertexAttribute(&formats->model_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, normal));
		AddVertexAttribute(&formats->model_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, tangent));
		AddVertexAttribute(&formats->model_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, bitangent));
	}
}
