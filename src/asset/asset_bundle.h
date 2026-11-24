#ifndef ASSET_ASSET_BUNDLE_H
#define ASSET_ASSET_BUNDLE_H

#include "core/types.h"
#include "os/job.h"
#include "asset/handle.h"

#include "container/hash_map.h"
#include "container/string.h"
#include "container/vector.h"

#include <set>

namespace ast {

class AssetSystem;

class AssetBundle {
public:
	explicit AssetBundle(const String &name);

	AssetBundle &Add(const String &file);
	AssetBundle &AddAll(const Vector<String> &files);

	[[nodiscard]] const String &GetName() const;
	[[nodiscard]] const Vector<String> &GetFiles() const;

private:
	String name;
	Vector<String> files;
};

class BundleLoader {
public:
	BundleLoader(AssetSystem *assets, const String &prefix);
	~BundleLoader();

	BundleLoader(const BundleLoader &) = delete;
	BundleLoader &operator=(const BundleLoader &) = delete;

	// start loading the wanted bundles,
	// bundles that aren't wanted anymore are handed out by
	// TakeUnwantedFilePaths l8r, once the loadign finishes.
	void SwitchTo(const Vector<const AssetBundle *> &wanted);

	bool DoUploads();

	void WaitUntilLoaded();
	float GetProgress() const;

	Handle Find(const String &file) const;

	// these bundles are forgotten but their files are still
	// loaded until unload(), so that anything built on top of
	// them can still be released first.
	Vector<String> TakeUnwantedFilePaths();

	void Unload(const String &file);

	void UnloadUnwanted();

private:
	void Request(const String &file);

	AssetSystem *assets;
	String prefix;
	job::Counter counter;

	HashMap<String, Handle> handles;

	std::set<const AssetBundle *> loaded_bundles;
	std::set<const AssetBundle *> unwanted_bundles;
};

}

#endif // ASSET_ASSET_BUNDLE_H
