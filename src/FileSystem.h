#ifndef REDFS_FILESYSTEM_H
#define REDFS_FILESYSTEM_H

#include <filesystem>
#include <regex>
#include <unordered_map>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "File.h"
#include "FileSystemStatus.h"
#include "FileSystemStorage.h"

namespace RedFS {

class FileSystem : public Red::IScriptable {
 private:
  using StorageMap =
    std::unordered_map<std::string, Red::Handle<FileSystemStorage>>;

  static RED4ext::PluginHandle handle;
  static RED4ext::Logger* logger;

  static std::filesystem::path game_path;
  static std::filesystem::path storages_path;

  static std::regex storage_name_rule;

  static StorageMap storages;
  static bool has_error;

  static bool request_directory(const std::filesystem::path& p_path);
  static bool migrate_directory(const std::filesystem::path& p_old_path,
                                const std::filesystem::path& p_new_path);

  static Red::Handle<FileSystemStorage> find_storage(const std::string& p_name);

 public:
  static void load(RED4ext::PluginHandle p_handle, RED4ext::Logger* p_logger);
  static void unload();

  static Red::Handle<FileSystemStorage> get_storage(const Red::CString& p_name);
  static Red::Handle<FileSystemStorage> get_shared_storage();

  RTTI_IMPL_TYPEINFO(RedFS::FileSystem);
  RTTI_IMPL_ALLOCATOR();
};

}  // namespace RedFS

RTTI_DEFINE_CLASS(RedFS::FileSystem, {
  RTTI_ALIAS("RedFileSystem.FileSystem");

  RTTI_METHOD(get_storage, "GetStorage");
  RTTI_METHOD(get_shared_storage, "GetSharedStorage");
});

#endif  //REDFS_FILESYSTEM_H
