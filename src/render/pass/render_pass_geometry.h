#ifndef RENDER_PASS_GEOMETRY_H
#define RENDER_PASS_GEOMETRY_H

/*
 * Calculates a normals and depth texture.
 * Useful for screen-space effects, like
 * SSAO.
 */

typedef struct R_GeometryPassData R_GeometryPassData;
struct R_GeometryPassData
{
};

R_PASS_RECORD_DEF(R_GeometryPassFn);


typedef struct R_GeometryRenderer R_GeometryRenderer;
struct R_GeometryRenderer
{
	G_Device *device;
	A_Registry *assets;

	A_Handle shader;
};

void R_GeometryRendererInit (R_GeometryRenderer *r);

void R_GeometryRender       (R_GeometryRenderer *r,
							 R_Graph *graph,
							 R_Blackboard *bb);

#endif // RENDER_PASS_GEOMETRY_H
