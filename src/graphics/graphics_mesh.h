#ifndef GRAPHICS_MESH_H
#define GRAPHICS_MESH_H

typedef enum G_IndexType
{
	G_IndexType_U16,
	G_IndexType_U32,
	G_IndexType_COUNT
}
G_IndexType;

internal inline VkIndexType G_IndexTypeToVk(G_IndexType type)
{
	switch (type)
	{
		case G_IndexType_U16:  return VK_INDEX_TYPE_UINT16;
		case G_IndexType_U32:  return VK_INDEX_TYPE_UINT32;
	}

	AssertTrue(false);

	return (VkIndexType)0;
}

internal inline u64 G_IndexTypeStride(G_IndexType type)
{
	switch (type)
	{
		case G_IndexType_U16:  return sizeof(u16);
		case G_IndexType_U32:  return sizeof(u32);
	}

	AssertTrue(false);

	return 0;
}

#endif // GRAPHICS_MESH_H
