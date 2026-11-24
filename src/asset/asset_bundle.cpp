#include "asset/asset_bundle.h"
#include "asset/asset_system.h"

ast::AssetBundle::AssetBundle(const String &name_)
	: name(name_)
{
}

ast::AssetBundle &ast::AssetBundle::Add(const String &file)
{
	files.push_back(file);
	return *this;
}

ast::AssetBundle &ast::AssetBundle::AddAll(const Vector<String> &more)
{
	for (const String &file : more)
		files.push_back(file);

	return *this;
}

const String &ast::AssetBundle::GetName() const
{
	return name;
}

const Vector<String> &ast::AssetBundle::GetFiles() const
{
	return files;
}

ast::BundleLoader::BundleLoader(AssetSystem *assets_, const String &prefix_)
	: assets(assets_)
	, prefix(prefix_)
	, counter(0)
{
}

ast::BundleLoader::~BundleLoader()
{
	WaitUntilLoaded();

	Vector<String> files;

	for (const auto &kv : handles)
		files.push_back(kv.first);

	for (const String &file : files)
		Unload(file);
}

void ast::BundleLoader::Request(const String &file)
{
	if (handles.count(file) != 0)
		return;

	handles[file] = assets->RequireAsset(prefix + file, &counter);
}

void ast::BundleLoader::Unload(const String &file)
{
	auto it = handles.find(file);

	if (it == handles.end())
		return;

	assets->DestroyAsset(it->second);
	handles.erase(it);
}

void ast::BundleLoader::SwitchTo(const Vector<const AssetBundle *> &wanted)
{
	std::set<const AssetBundle *> next(wanted.begin(), wanted.end());

	for (const AssetBundle *bundle : next) {
		unwanted_bundles.erase(bundle);

		bool newly_loaded = loaded_bundles.insert(bundle).second;

		if (newly_loaded) {
			for (const String &file : bundle->GetFiles())
				Request(file);
		}
	}

	for (const AssetBundle *bundle : loaded_bundles) {
		if (next.count(bundle) == 0)
			unwanted_bundles.insert(bundle);
	}
}

bool ast::BundleLoader::DoUploads()
{
	assets->FlushUploads();
	return counter.Value() == 0;
}

void ast::BundleLoader::WaitUntilLoaded()
{
	assets->WaitForLoad(&counter);
}

float ast::BundleLoader::GetProgress() const
{
	if (handles.empty())
		return 1.f;

	int ready = 0;

	for (const auto &kv : handles) {
		LoadState state = assets->GetLoadState(kv.second);

		if (state == LoadState_Ready || state == LoadState_Failed)
			ready++;
	}

	return (float)ready / (float)handles.size();
}

ast::Handle ast::BundleLoader::Find(const String &file) const
{
	auto it = handles.find(file);

	if (it == handles.end())
		return Handle();

	return it->second;
}

Vector<String> ast::BundleLoader::TakeUnwantedFilePaths()
{
	Vector<String> files;

	if (counter.Value() != 0)
		return files;

	for (const AssetBundle *bundle : unwanted_bundles) {
		for (const String &file : bundle->GetFiles())
			files.push_back(file);

		loaded_bundles.erase(bundle);
	}

	unwanted_bundles.clear();

	return files;
}

void ast::BundleLoader::UnloadUnwanted()
{
	for (const String &file : TakeUnwantedFilePaths())
		Unload(file);
}
