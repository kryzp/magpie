
internal void AddVertexBinding(VertexFormat *vertex, u32 stride,
			       VkVertexInputRate input_rate)
{
	if (input_rate == VK_VERTEX_INPUT_RATE_VERTEX)
		vertex->vertex_size = stride;
	else if (input_rate == VK_VERTEX_INPUT_RATE_INSTANCE)
		vertex->instance_size = stride;

	VkVertexInputBindingDescription *b = vertex->bindings + vertex->binding_count;
	b->binding = vertex->binding_count;
	b->stride = stride;
	b->inputRate = input_rate;

	vertex->binding_count++;
}

internal void AddVertexAttribute(VertexFormat *vertex, VkFormat format,
				 u32 offset)
{
	VkVertexInputAttributeDescription *a = vertex->attributes + vertex->attribute_count;
	a->binding = vertex->binding_count - 1;
	a->location = vertex->attribute_count;
	a->format = format;
	a->offset = offset;

	vertex->attribute_count++;
}
