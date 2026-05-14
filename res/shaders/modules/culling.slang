module culling;

import scene;

namespace culling
{
	public interface IVolumeTest
	{
		bool Test(float3 world_centre, float radius);
	}

	public struct FrustumTest : IVolumeTest
	{
		public float4 planes[6]; // xyz = normal, w = distance

		public bool Test(float3 c, float r)
		{
			for (int i = 0; i < 6; i++)
			{
				if (dot(planes[i].xyz, c) + planes[i].w < -r)
					return false;
			}
			return true;
		}
	}

	public struct SphereTest : IVolumeTest
	{
		public float4 sphere; // xyz = position, w = radius

		public bool Test(float3 c, float r)
		{
			float3 delta = c - sphere.xyz;
			float dist_sq = dot(delta, delta);
			float radius_sum = r + sphere.w;
			return dist_sq < radius_sum * radius_sum;
		}
	}
	
	public FrustumTest MakeFrustumTest(float4 planes[6])
	{
		FrustumTest t;

		for (int i = 0; i < 6; i++)
			t.planes[i] = planes[i];
		
		return t;
	}

	public SphereTest MakeSphereTest(float4 sphere)
	{
		SphereTest t;
		
		t.sphere = sphere;
		
		return t;
	}

	public static uint32_t ALPHA_FILTER_OPAQUE_ONLY = 0;
	public static uint32_t ALPHA_FILTER_ALPHA_ONLY  = 1;
	public static uint32_t ALPHA_FILTER_ALL         = 2;

	public bool PassesAlphaFilter(uint32_t alpha_mode, uint32_t filter)
	{
		if (filter == ALPHA_FILTER_OPAQUE_ONLY)  return alpha_mode != scene::ALPHA_MODE_BLEND;
		if (filter == ALPHA_FILTER_ALPHA_ONLY)   return alpha_mode == scene::ALPHA_MODE_BLEND;
		if (filter == ALPHA_FILTER_ALL)          return true;
		
		return true;
	}

	public bool IsVisibleAgainst<T : IVolumeTest>(scene::ObjectData obj, T test)
	{
		float3 world_centre = mul(obj.model_matrix, float4(obj.sphere_bounds.xyz, 1.0)).xyz;

		float scale_x = length(obj.model_matrix[0].xyz);
		float scale_y = length(obj.model_matrix[1].xyz);
		float scale_z = length(obj.model_matrix[2].xyz);
		
		float max_scale = max(scale_x, max(scale_y, scale_z));

		float radius = obj.sphere_bounds.w * max_scale;

		return test.Test(world_centre, radius);
	}

	public scene::IndirectDraw GetDrawFor(scene::ObjectData *objects,
	                                      scene::RenderMesh *meshes,
	                                      uint gid)
	{
		scene::RenderMesh mesh = meshes[objects[gid].mesh_index];

		scene::IndirectDraw draw;
		draw.index_count = mesh.index_count;
		draw.instance_count = 1;
		draw.first_index = mesh.first_index;
		draw.vertex_offset = 0; // We use BDA instead.
		draw.first_instance = gid;
		return draw;
	}
}
