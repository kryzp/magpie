
static E_Transform E_TransformIdentity(void)
{
	E_Transform transform = {0};
	transform.position = v3x(0.f);
	transform.rotation = V4QuatIdentity();
	transform.scale = v3x(1.f);
	transform.origin = v3x(0.f);

	E_TransformRecompute(&transform);
	
	return transform;
}

static void E_TransformRecompute(E_Transform *transform)
{
	transform->matrix = M4Transform(
		transform->position,
		transform->rotation,
		transform->scale,
		transform->origin
	);

	transform->dirty = false;
}

static m4 E_TransformMatrix(E_Transform *transform)
{
	if (transform->dirty)
		E_TransformRecompute(transform);

	return transform->matrix;
}

static void E_TransformSetPosition(E_Transform *transform, v3 position)
{
	transform->position = position;
	transform->dirty = true;
}

static void E_TransformMoveBy(E_Transform *transform, v3 by)
{
	transform->position = V3Add(transform->position, by);
	transform->dirty = true;
}

static void E_TransformSetRotation(E_Transform *transform, v4 rotation)
{
	transform->rotation = rotation;
	transform->dirty = true;
}

static void E_TransformSetScale(E_Transform *transform, v3 scale)
{
	transform->scale = scale;
	transform->dirty = true;
}

static void E_TransformSetOrigin(E_Transform *transform, v3 origin)
{
	transform->origin = origin;
	transform->dirty = true;
}
