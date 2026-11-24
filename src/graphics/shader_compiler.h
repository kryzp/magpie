#ifndef GRAPHICS_SHADER_COMPILER_H
#define GRAPHICS_SHADER_COMPILER_H

#include "core/types.h"
#include "core/log.h"
#include "os/job.h"
#include "graphics/shader.h"
#include "container/string.h"
#include "container/vector.h"

#define GRAPHICS_SHADER_COMPILER_MAX_STAGES 8

struct ISlangBlob;

namespace slang {
struct IGlobalSession;
}

namespace gfx {

struct ShaderCompiledStages {
	bool failed = true;
	Vector<ShaderBytecode> bytecodes;
};

class ShaderCompiler {
public:
	explicit ShaderCompiler(LogChannel log_channel);
	~ShaderCompiler();

	ShaderCompiler(const ShaderCompiler &) = delete;
	ShaderCompiler &operator=(const ShaderCompiler &) = delete;

	static ShaderCompiler *Get();

	ShaderCompiledStages Compile(
		const String &source_path,
		const Vector<String> &search_paths
	);

private:
	void LogDiagnostics(const char *context, const char *source, ISlangBlob *diag);

	LogChannel log_channel;
	job::FiberMutex mutex;
	slang::IGlobalSession *global_session;
};

}

#endif // GRAPHICS_SHADER_COMPILER_H
