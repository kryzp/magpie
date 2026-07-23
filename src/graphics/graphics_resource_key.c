
internal G_ResourceKey G_ResourceKeyNull(void)
{
	G_ResourceKey null_key = {0};
	return null_key;
}

internal b32 G_ResourceKeyIsNull(G_ResourceKey key)
{
	return key.value == 0;
}

internal b32 G_ResourceKeyMatch(G_ResourceKey a, G_ResourceKey b)
{
	return a.value == b.value;
}
