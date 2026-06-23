#ifndef RENDER_PASS_BRDF_LUT_H
#define RENDER_PASS_BRDF_LUT_H

typedef struct R_BRDFLutPassData R_BRDFLutPassData;
struct R_BRDFLutPassData
{
	G_ShaderKey shader;
};

static R_PASS_RECORD_DEF(R_BRDFLutPassFn);

#endif // RENDER_PASS_BRDF_LUT_H
