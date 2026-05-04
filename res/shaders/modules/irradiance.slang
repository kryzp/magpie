module irradiance;

import math;

namespace irradiance
{
	public struct ProbeGridInfo
	{
		public float3 grid_min;
		public float  _pad0;

		public float3 grid_max;
		public float  _pad1;

		public float3 cell_size;
		public float  _pad2;

		public uint count_x;
		public uint count_y;
		public uint count_z;
		public uint total_probe_count;
	};

	public struct ProbeSH
	{
		public float4 coefficients[9];
	};

	public static const float SH_C0 = 0.282095;
	public static const float SH_C1 = 0.488603;
	public static const float SH_C2 = 1.092548;
	public static const float SH_C3 = 0.315392;
	public static const float SH_C4 = 0.546274;

	public static const float SH_COS_CONV[9] = {
		math::PI,             // l=0
		math::PI*2.0/3.0,     // l=1
		math::PI*2.0/3.0,
		math::PI*2.0/3.0,
		math::PI/4.0,         // l=2
		math::PI/4.0,
		math::PI/4.0,
		math::PI/4.0,
		math::PI/4.0,
	};

	public static const float MAX_REFLECTION_LOD = 5.0;

	// Ramamoorthi & Hanrahan (2001) "An Efficient Representation for Irradiance Environment Maps".
	public void SH_EvaluateBasis(float3 d, out float basis[9])
	{
		// l=0
		basis[0] = SH_C0;

		// l=1
		basis[1] = SH_C1 * d.y;
		basis[2] = SH_C1 * d.z;
		basis[3] = SH_C1 * d.x;

		// l=2
		basis[4] = SH_C2 * d.x * d.y;
		basis[5] = SH_C2 * d.y * d.z;
		basis[6] = SH_C3 * (3.0 * d.z * d.z - 1.0);
		basis[7] = SH_C2 * d.x * d.z;
		basis[8] = SH_C4 * (d.x * d.x - d.y * d.y);
	}

	public float3 SH_Evaluate(ProbeSH probe, float3 normal)
	{
		float basis[9];
		SH_EvaluateBasis(normal, basis);

		float3 irradiance = float3(0, 0, 0);
		
		for (uint i = 0; i < 9; i++)
			irradiance += probe.coefficients[i].xyz * basis[i];

		return max(irradiance, float3(0, 0, 0));
	}

	/* bro FUCK my life bro */
	/* credit to some academic man i cant remember atp who cares */
	public float3 SampleIrradianceGrid(float3 world_pos,
	                                   float3 normal,
	                                   ProbeGridInfo grid,
	                                   ProbeSH *probe_data)
	{
		float3 local = world_pos - grid.grid_min;

		float3 grid_coord = float3((grid.cell_size.x > 0) ? local.x / grid.cell_size.x : 0,
		                           (grid.cell_size.y > 0) ? local.y / grid.cell_size.y : 0,
		                           (grid.cell_size.z > 0) ? local.z / grid.cell_size.z : 0);

		// saturat
		grid_coord.x = clamp(grid_coord.x, 0, float(grid.count_x - 1));
		grid_coord.y = clamp(grid_coord.y, 0, float(grid.count_y - 1));
		grid_coord.z = clamp(grid_coord.z, 0, float(grid.count_z - 1));

		uint3 lo = uint3(floor(grid_coord));
		uint3 hi = min(lo + 1, uint3(grid.count_x - 1,
		                             grid.count_y - 1,
		                             grid.count_z - 1));

		float3 frac = grid_coord - float3(lo);

#define PROBE_INDEX(ix, iy, iz) ((iz) * grid.count_x * grid.count_y + (iy) * grid.count_x + (ix))

		float3 c000 = SH_Evaluate(probe_data[PROBE_INDEX(lo.x, lo.y, lo.z)], normal);
		float3 c100 = SH_Evaluate(probe_data[PROBE_INDEX(hi.x, lo.y, lo.z)], normal);
		float3 c010 = SH_Evaluate(probe_data[PROBE_INDEX(lo.x, hi.y, lo.z)], normal);
		float3 c110 = SH_Evaluate(probe_data[PROBE_INDEX(hi.x, hi.y, lo.z)], normal);
		float3 c001 = SH_Evaluate(probe_data[PROBE_INDEX(lo.x, lo.y, hi.z)], normal);
		float3 c101 = SH_Evaluate(probe_data[PROBE_INDEX(hi.x, lo.y, hi.z)], normal);
		float3 c011 = SH_Evaluate(probe_data[PROBE_INDEX(lo.x, hi.y, hi.z)], normal);
		float3 c111 = SH_Evaluate(probe_data[PROBE_INDEX(hi.x, hi.y, hi.z)], normal);

#undef PROBE_INDEX

		float3 c00 = lerp(c000, c100, frac.x);
		float3 c10 = lerp(c010, c110, frac.x);
		float3 c01 = lerp(c001, c101, frac.x);
		float3 c11 = lerp(c011, c111, frac.x);

		float3 c0 = lerp(c00, c10, frac.y);
		float3 c1 = lerp(c01, c11, frac.y);

		return lerp(c0, c1, frac.z);
	}
}
