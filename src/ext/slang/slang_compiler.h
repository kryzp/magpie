#ifndef SLANG_COMPILER_H
#define SLANG_COMPILER_H

/*
 * SLANG has no C bindings to my knowledge (if they do exist
 * then fuck i guess), so I went ahead and wrote the shittiest
 * most barebones layer exposing some basic functionality, which
 * gets compiled seperately and linked, similar to VMA.
 *
 * Will need to work more on this, but it gets the job done
 * for now.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SLANG_MAX_STAGES 8

void SLANG_Init(void **out_global_session);
void SLANG_Shutdown(void *global_session);

typedef void SLANG_LogFn(const char *context,
						 const char *source,
						 const char *message,
						 void *user_data);

typedef struct SLANG_StageResult SLANG_StageResult;
struct SLANG_StageResult
{
	void *bytes;
	uint64_t size;
};

typedef struct SLANG_CompileResult SLANG_CompileResult;
struct SLANG_CompileResult
{
	uint32_t failed;
	uint32_t stage_count;
	SLANG_StageResult stages[SLANG_MAX_STAGES];
};

SLANG_CompileResult SLANG_Compile(void *global_session,
								  const char *source_path,
								  uint32_t search_path_count,
								  const char *const *search_paths,
								  SLANG_LogFn *log_fn, void *user_data);

void SLANG_FreeResult(SLANG_CompileResult *result);

#ifdef __cplusplus
}
#endif

#endif /* SLANG_COMPILER_H */
