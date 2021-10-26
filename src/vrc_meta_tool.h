#ifndef VRC_PHOTO_ALBUM2_VRC_META_TOOL_H
#define VRC_PHOTO_ALBUM2_VRC_META_TOOL_H

#include <netinet/in.h>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace vrc_photo_album2::meta_tool {

namespace filesystem = std::filesystem;

typedef struct {
  // TODO: clang++でchrono::local_seconds周りの実装が充実してきたらこっちにする
  std::optional<std::string> date;
  std::optional<std::string> photographer;
  std::optional<std::string> world;
  std::map<std::string, std::optional<std::string>> users;
} meta_data;

struct header {
  uint32_t size;
  char type[4];
};

class chunk_s {
public:
  header head_;
  char* data_;
  uint32_t size() {
    return ntohl(this->head_.size);
  };
};

class chunk_util {
public:
  chunk_util(filesystem::path path);
  decltype(auto) read();
  // decltype(auto) write();
  // decltype(auto) create_chunk();

private:
  decltype(auto) parse_chunk(chunk_s& chunk);
  filesystem::path path_;
};

class meta_tool {
public:
  std::string date() const;
  std::string readable_date() const;
  std::string photographer() const;
  std::string world() const;
  std::map<std::string, std::optional<std::string>> users() const;
  bool has_any() const;
  bool has_date() const;
  bool has_readable_date() const;
  bool has_photographer() const;
  bool has_world() const;
  bool has_users() const;
  // decltype(auto) users_begin() const;
  // decltype(auto) users_end() const;
  void set_date(std::optional<std::string> date);
  void set_photographer(std::optional<std::string> photographer);
  void set_world(std::optional<std::string> world);
  void add_user(std::string user);
  void delete_user(std::string user_name);
  void clear_users();
  void read(filesystem::path path);

private:
  meta_data data_;
};
} // namespace vrc_photo_album2::meta_tool
#endif
