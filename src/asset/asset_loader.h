#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include "core/types.h"
#include "core/log.h"
#include "asset/handle.h"
#include "asset/types.h"
#include "graphics/resource_key.h"
#include "container/string.h"
#include "container/vector.h"

namespace gfx {
class CmdBuffer;
}

namespace ast {

struct Metadata {
	String path;
	u64 last_write_time = 0;
	bool srgb = false;
};

struct ResidencyLevel {
	int lod = 0;

	int Compare(const ResidencyLevel &other) const
	{
		return lod - other.lod;
	}
};

struct LoadResidencyCtx {
	float distance = 0.f;
};

struct LoaderContext {
	LogChannel log_channel;
	Metadata metadata;
	ResidencyLevel current_residency;
	ResidencyLevel target_residency;

	/*
	String SystemFilePath() const
	{
		return AssetSystem::Get()->GetSystemFilePath(metadata.path);
	}
	*/
};

struct LoadData {
	virtual ~LoadData() = default;
};

struct LoadResult {
	LoadData *user_data = nullptr; // owned and deleted once the upload is finished.
	u64 stage_size = 0;
	bool failed = false;
	Vector<Handle> dependencies;
};

class IAssetLoader {
public:
	virtual ~IAssetLoader() = default;

	virtual bool IsStreamable() const
	{
		return false;
	}

	virtual const Vector<String> &GetFileExtensions() const = 0;

	virtual LoadResult Load(const LoaderContext &ctx) = 0;

	virtual void Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset)
	{
	}

	virtual void UploadGPU(
		const LoaderContext &ctx,
		LoadResult *result,
		Asset *asset,
		const gfx::CmdBuffer &cmd,
		gfx::ResourceKey stage,
		u64 stage_offset
	)
	{
	}

	virtual void DestroyIntermediateResources(LoadResult *result)
	{
	}

	virtual void DestroyAsset(Asset *asset)
	{
	}

	virtual ResidencyLevel CalculateResidency(const LoadResidencyCtx &ctx)
	{
		return ResidencyLevel();
	}
};

class TextureLoader : public IAssetLoader {
public:
	bool IsStreamable() const override
	{
		return true;
	}

	const Vector<String> &GetFileExtensions() const override;

	LoadResult Load(const LoaderContext &ctx) override;
	void Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset) override;
	void UploadGPU(const LoaderContext &ctx, LoadResult *result, Asset *asset, const gfx::CmdBuffer &cmd, gfx::ResourceKey stage, u64 stage_offset) override;
	void DestroyIntermediateResources(LoadResult *result) override;
	void DestroyAsset(Asset *asset) override;
	ResidencyLevel CalculateResidency(const LoadResidencyCtx &ctx) override;
};

class ShaderLoader : public IAssetLoader {
public:
	const Vector<String> &GetFileExtensions() const override;

	LoadResult Load(const LoaderContext &ctx) override;
	void Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset) override;
	void DestroyAsset(Asset *asset) override;
};

class ModelLoader : public IAssetLoader {
public:
	const Vector<String> &GetFileExtensions() const override;

	LoadResult Load(const LoaderContext &ctx) override;
	void Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset) override;
	void UploadGPU(const LoaderContext &ctx, LoadResult *result, Asset *asset, const gfx::CmdBuffer &cmd, gfx::ResourceKey stage, u64 stage_offset) override;
	void DestroyAsset(Asset *asset) override;
};

class SoundLoader : public IAssetLoader {
public:
	const Vector<String> &GetFileExtensions() const override;

	LoadResult Load(const LoaderContext &ctx) override;
	void Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset) override;
	void DestroyAsset(Asset *asset) override;
};

class ScriptLoader : public IAssetLoader {
public:
	const Vector<String> &GetFileExtensions() const override;

	LoadResult Load(const LoaderContext &ctx) override;
	void Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset) override;
};

}

#endif // ASSET_LOADER_H
