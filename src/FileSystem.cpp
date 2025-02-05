#include "FileSystem.h"
#include "FileSystemStorage.h"

#include "Utils.h"

namespace RedFS {

RED4ext::PluginHandle FileSystem::handle = nullptr;
RED4ext::Logger* FileSystem::logger = nullptr;

std::filesystem::path FileSystem::game_path;
std::filesystem::path FileSystem::storages_path;

std::regex FileSystem::storage_name_rule("[A-Za-z]{3,24}");

FileSystem::StorageMap FileSystem::storages{};
bool FileSystem::has_error = true;

void FileSystem::load(RED4ext::PluginHandle p_handle,
                      RED4ext::Logger* p_logger) {
  handle = p_handle;
  logger = p_logger;
  auto path = std::filesystem::absolute(".");

  game_path = path.parent_path().parent_path();
  storages_path = game_path / "r6" / "storages";
  bool is_present = request_directory(storages_path);

  if (!is_present) {
    has_error = true;
    logger->ErrorF(handle, "Failed to create directory at \"%s\".",
                   storages_path.string().c_str());
    logger->Error(handle, "RedFileSystem has been disabled.");
    return;
  }
  auto old_path =
    game_path / "red4ext" / "plugins" / "RedFileSystem" / "storages";

  if (!migrate_directory(old_path, storages_path)) {
    has_error = true;
    logger->WarnF(handle, R"(Failed to migrate directory from "%s" to "%s".)",
                  old_path.string().c_str(), storages_path.string().c_str());
    logger->Warn(handle, "You need to manually move content yourself.");
    return;
  }
  has_error = false;
  logger->Info(handle, "RedFileSystem has been enabled.");
}

void FileSystem::unload() {
  std::error_code error;
  auto old_path =
    game_path / "red4ext" / "plugins" / "RedFileSystem" / "storages";

  std::filesystem::remove_all(old_path, error);
  storages.clear();
  has_error = true;
  logger->Info(handle, "RedFileSystem has been terminated.");
  handle = nullptr;
  logger = nullptr;
}

Red::Handle<FileSystemStorage> FileSystem::get_storage(
  const Red::CString& p_name) {
  if (has_error) {
    logger->Error(handle, "RedFileSystem is disabled for all mods.");
    return {};
  }
  std::string name = p_name.c_str();

  if (!regex_match(name, storage_name_rule) ||
      equals_insensitive(name, "shared")) {
    logger->ErrorF(handle, "Name of storage \"%s\" is not allowed.",
                   name.c_str());
    logger->Error(handle, "See the documentation to fix this issue.");
    return {};
  }
  auto storage = find_storage(name);

  if (storage) {
    storage->revoke_permission();
    logger->ErrorF(handle,
                   "Attempt to access storage \"%s\" several times. "
                   "Only one mod can access its own storage with "
                   "RedFileSystem. Access to this storage has been permanently "
                   "revoked for this session.",
                   name.c_str());
    return {};
  }
  auto path = storages_path / name;
  bool is_present = request_directory(path);

  if (!is_present) {
    logger->ErrorF(handle, "Failed to create storage \"%s\".", name.c_str());
    return {};
  }
  storage = Red::MakeHandle<FileSystemStorage>(name, path);
  storages[name] = storage;
  logger->InfoF(handle, "Access to storage \"%s\" has been granted.",
                name.c_str());
  return storage;
}

Red::Handle<FileSystemStorage> FileSystem::get_shared_storage() {
  if (has_error) {
    logger->Error(handle, "RedFileSystem is disabled.");
    return {};
  }
  constexpr auto name = "shared";
  auto storage = find_storage(name);

  if (storage) {
    logger->Info(handle, "Access to shared storage has been granted.");
    return storage;
  }
  auto path = storages_path / name;
  bool is_present = request_directory(path);

  if (!is_present) {
    logger->Error(handle, "Failed to create shared storage.");
    return {};
  }
  storage = Red::MakeHandle<FileSystemStorage>(name, path);
  storages[name] = storage;
  logger->Info(handle, "Access to shared storage has been granted.");
  return storage;
}

bool FileSystem::request_directory(const std::filesystem::path& p_path) {
  std::error_code error;
  bool is_present = std::filesystem::exists(p_path, error);

  if (error) {
    return false;
  }
  if (is_present) {
    return true;
  }
  is_present = std::filesystem::create_directory(p_path, error);
  if (!is_present || error) {
    return false;
  }
  return true;
}

bool FileSystem::migrate_directory(const std::filesystem::path& p_old_path,
                                   const std::filesystem::path& p_new_path) {
  std::error_code error;
  bool has_old_path = std::filesystem::exists(p_old_path, error);

  if (error) {
    return false;
  }
  if (!has_old_path) {
    return true;
  }
  std::filesystem::copy_options options =
    std::filesystem::copy_options::overwrite_existing |
    std::filesystem::copy_options::recursive;

  std::filesystem::copy(p_old_path, p_new_path, options, error);
  if (error) {
    logger->ErrorF(handle, "Could not migrate \"storages\" due to: %s.",
                   error.message().c_str());
    return false;
  }
  // NOTE: remove old path in unload() to be backward compatible with
  //       RED4ext v1.24.3
  return true;
}

Red::Handle<FileSystemStorage> FileSystem::find_storage(
  const std::string& p_name) {
  for (const auto& storage : storages) {
    if (equals_insensitive(storage.first, p_name)) {
      return storage.second;
    }
  }
  return {};
}

}  // namespace RedFS