
static Transform TransformIdentity(void)
{
	Transform transform = {0};
	transform.position = v3x(0.f);
	transform.rotation = V4QuatIdentity();
	transform.scale = v3x(1.f);
	transform.origin = v3x(0.f);

	TransformRecompute(&transform);
	
	return transform;
}

static void TransformRecompute(Transform *transform)
{
	transform->matrix = M4Transform(
		transform->position,
		transform->rotation,
		transform->scale,
		transform->origin
	);

	transform->dirty = false;
}

static m4 TransformMatrix(Transform *transform)
{
	if (transform->dirty)
		TransformRecompute(transform);

	return transform->matrix;
}

static void TransformSetPosition(Transform *transform, v3 position)
{
	transform->position = position;
	transform->dirty = true;
}

static void TransformMoveBy(Transform *transform, v3 by)
{
	transform->position = V3Add(transform->position, by);
	transform->dirty = true;
}

static void TransformSetRotation(Transform *transform, v4 rotation)
{
	transform->rotation = rotation;
	transform->dirty = true;
}

static void TransformSetScale(Transform *transform, v3 scale)
{
	transform->scale = scale;
	transform->dirty = true;
}

static void TransformSetOrigin(Transform *transform, v3 origin)
{
	transform->origin = origin;
	transform->dirty = true;
}
