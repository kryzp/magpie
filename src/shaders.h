
typedef struct Shaders {
#define GraphicsShaderDef(__path_vert, __path_comp, __field) ShaderProgram __field;
#define ComputeShaderDef(__path_comp, __field) ShaderProgram __field;
#include "shaders.inc"
#undef GraphicsShaderDef
#undef ComputeShaderDef
} Shaders;
