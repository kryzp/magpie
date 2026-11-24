#ifndef ASSET_ASSET_SYSTEM_H
#define ASSET_ASSET_SYSTEM_H

// MAJOR INFORMATION / TODO
// ASSET STREAMING ISN'T IMPLEMENTED YET!
// so do that...
// at some point...
// :(

#include "core/types.h"
#include "core/log.h"
#include "core/pool.h"
#include "os/job.h"
#include "os/spin_lock.h"
#include "asset/handle.h"
#include "asset/types.h"
#include "asset/asset_loader.h"
#include "container/string.h"
#include "container/vector.h"
#include "container/hash_map.h"

#define ASSETS_MAX_RECORDS                    2048
#define ASSETS_UPLOAD_QUEUE_MAX_SIZE          1024
#define ASSETS_GPU_UPLOAD_CHUNK_MAX_SIZE      Megabytes(512)

namespace ast {

enum LoadState {
	LoadState_Unloaded,
	LoadState_Loading,
	LoadState_Ready,
	LoadState_Failed,
	LoadState_MaxEnum
};

struct Record {
	unsigned id = 0;
	unsigned generation = 0;
	AssetType type = AssetType_Null;

	Asset asset;
	LoadState load_state = LoadState_Unloaded;
	Metadata metadata;

	bool is_reloading = false;
	bool is_streaming = false;

	ResidencyLevel current_residency;
	ResidencyLevel target_residency;
};

struct MountPoint {
	String prefix;
	String directory;
};

struct TypeLoader {
	IAssetLoader *loader = nullptr;
	LogChannel log_channel;
	Handle fallback;
};

struct Upload {
	Record *record = nullptr;
	LoadResult result;
};

struct LoadJobParam {
	Record *record = nullptr;
	job::Counter *counter = nullptr;
};

class AssetSystem {
public:
	explicit AssetSystem(LogChannel log_channel);
	~AssetSystem();

	AssetSystem(const AssetSystem &) = delete;
	AssetSystem &operator=(const AssetSystem &) = delete;

	static AssetSystem *Get();

	void SetFallback(Handle handle);

	void PollHotReloads();
	void FlushUploads();

	void DoAssetStreaming();

	void SetStreamingState(Handle handle, const LoadResidencyCtx &residency_ctx);

	AssetType GetAssetTypeFromPath(const String &path);

	Handle HandleFromFilePath(const String &path);

	Asset *GetOrFallback(Handle handle);
	Asset *GetOrBreak(Handle handle);

	Handle RequireAsset(const String &path, job::Counter *counter);
	Handle RequireAssetBlocking(const String &path);

	void DestroyAsset(Handle handle);

	void WaitForLoad(job::Counter *counter);

	void Mount(const String &prefix, const String &directory);
	String GetSystemFilePath(const String &path) const;

	void AddShaderSearchPath(const String &directory);
	const Vector<String> &GetShaderSearchPaths() const;

	LoadState GetLoadState(Handle handle);

	Record *GetRecord(Handle handle);

	void MarkTextureAsSRGB(Handle handle);

	LogChannel GetLogChannel() const
	{
		return log_channel;
	}

private:
	static void LoadJob(void *param);

	Handle PathMapFind(const String &path);
	void PathMapInsert(const String &path, Handle handle);
	void PathMapRemove(const String &path);

	Record *AllocRecord(AssetType type);
	void FreeRecord(Record *record);

	LoadJobParam *AllocLoadParam();
	void FreeLoadParam(LoadJobParam *param);

	LogChannel log_channel;

	Record *records;
	DensePool record_pool;
	SpinLock registry_lock;

	TypeLoader loaders[AssetType_MaxEnum];

	TextureLoader texture_loader;
	ShaderLoader shader_loader;
	ModelLoader model_loader;
	SoundLoader sound_loader;
	ScriptLoader script_loader;

	HashMap<String, Handle> path_map;

	Vector<MountPoint> mount_points;
	Vector<String> shader_search_paths;

	SpinLock upload_lock;
	Vector<Upload> upload_queue;

	SpinLock param_lock;
	Vector<LoadJobParam *> load_params;
};

}

#endif // ASSET_ASSET_SYSTEM_H
