#ifndef RENDER_PASS_GENERATE_MIPS_H
#define RENDER_PASS_GENERATE_MIPS_H

typedef struct R_GenerateMipsPassData R_GenerateMipsPassData;
struct R_GenerateMipsPassData
{
	GFX_TextureKey texture;
};

R_PASS_RECORD_DEF(R_GenerateMipsPass);

#endif // RENDER_PASS_GENERATE_MIPS_H
