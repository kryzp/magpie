
internal v3 P_RaycastCalcFinalPosition(const P_Raycast *ray)
{
    return P_RaycastCalcPositionAt(ray, ray->time);
}

internal v3 P_RaycastCalcPositionAt(const P_Raycast *ray, f32 t)
{
    return V3Add(ray->start_position, V3MulF32(ray->direction, t));
}
