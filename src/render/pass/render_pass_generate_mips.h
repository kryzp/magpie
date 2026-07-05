#ifndef RENDER_PASS_GENERATE_MIPS_H
#define RENDER_PASS_GENERATE_MIPS_H

typedef struct R_GenerateMipsPassData R_GenerateMipsPassData;
struct R_GenerateMipsPassData
{
	G_TextureKey texture;
};

static R_PASS_RECORD_DEF(R_GenerateMipsPassFn);

#endif // RENDER_PASS_GENERATE_MIPS_H
