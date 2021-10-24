#include "vrc_meta_tool.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <boost/format.hpp>
#include <png.h>

namespace vrc_photo_album2::meta_tool {

chunk_util::chunk_util(filesystem::path path) {
  fp = std::fopen(path.c_str(), "rb");
  if (!fp) {
    std::runtime_error("file can not open.");
  }
}

chunk_util::~chunk_util() {
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
  std::fclose(fp);
}

decltype(auto) chunk_util::parse_chunk(png_unknown_chunk chunk) {
  // chunk.dataは終端文字が入っていないため部分文字列を取得する
  return std::make_tuple(
      std::string(reinterpret_cast<char*>(chunk.name)),
      std::string(reinterpret_cast<char*>(chunk.data)).substr(0, chunk.size));
}

decltype(auto) chunk_util::read() {
  constexpr int png_header_size = 8;

  png_byte header[png_header_size];
  std::fread(header, 1, png_header_size, fp);
  if (png_sig_cmp(header, 0, png_header_size)) throw std::invalid_argument("not png file.");

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr) throw std::runtime_error("failed to png_create_read_struct.");

  auto meta_chunks = std::make_unique<std::vector<std::tuple<std::string, std::string>>>();

  if (setjmp(png_jmpbuf(png_ptr))) {
    return meta_chunks;
  }

  info_ptr = png_create_info_struct(png_ptr);
  end_ptr  = png_create_info_struct(png_ptr);
  if (!info_ptr || !end_ptr) throw std::invalid_argument("failed to png_create_info_struct.");

  if (!fp) {
    std::cout << "not fp" << std::endl;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, png_header_size);

  // unknown chunkの定義
  constexpr png_byte vrc_meta_chunks[] = {'v', 'r', 'C', 'd', '\0', 'v', 'r', 'C', 'p', '\0',
                                          'v', 'r', 'C', 'w', '\0', 'v', 'r', 'C', 'u', '\0'};

  png_set_keep_unknown_chunks(png_ptr, 3, vrc_meta_chunks, sizeof(vrc_meta_chunks) / 5);

  png_read_update_info(png_ptr, info_ptr);
  png_read_info(png_ptr, info_ptr);
  png_read_end(png_ptr, end_ptr);

  png_unknown_chunkp info_unknown_ptr;
  png_unknown_chunkp end_unknown_ptr;
  auto info_unknown_num =
      static_cast<int>(png_get_unknown_chunks(png_ptr, info_ptr, &info_unknown_ptr));
  auto end_unknown_num =
      static_cast<int>(png_get_unknown_chunks(png_ptr, end_ptr, &end_unknown_ptr));

  for (int i = 0; i < info_unknown_num; i++) {
    auto chunk = parse_chunk(info_unknown_ptr[i]);
    meta_chunks->push_back(chunk);
  }
  for (int i = 0; i < end_unknown_num; i++) {
    auto chunk = parse_chunk(end_unknown_ptr[i]);
    meta_chunks->push_back(chunk);
  }

  return meta_chunks;
}

// decltype(auto) chunk_util::write() {}

// decltype(auto) chunk_util::create_chunk() {}

std::string meta_tool::date() const {
  return data_.date.value_or("");
}

std::string meta_tool::readable_date() const {
  const std::string year   = data_.date.value().substr(0, 4);
  const std::string month  = data_.date.value().substr(4, 2);
  const std::string day    = data_.date.value().substr(6, 2);
  const std::string hour   = data_.date.value().substr(8, 2);
  const std::string minute = data_.date.value().substr(10, 2);
  const std::string second = data_.date.value().substr(12, 2);
  // const std::string milli  = data_.date.value().substr(14, 3);

  return (boost::format("%04d-%02d-%02d %02d:%02d:%02d") % year % month % day % hour % minute %
          second)
      .str();
}

std::string meta_tool::photographer() const {
  return data_.photographer.value_or("");
}

std::string meta_tool::world() const {
  return data_.world.value_or("");
}

std::map<std::string, std::optional<std::string>> meta_tool::users() const {
  return data_.users;
}

bool meta_tool::has_any() const {
  return has_date() || has_photographer() || has_world() || has_users();
}

bool meta_tool::has_date() const {
  return data_.date.has_value();
}

bool meta_tool::has_readable_date() const {
  return data_.date.has_value();
}

bool meta_tool::has_photographer() const {
  return data_.photographer.has_value();
}

bool meta_tool::has_world() const {
  return data_.world.has_value();
}

bool meta_tool::has_users() const {
  return !data_.users.empty();
}

void meta_tool::set_date(std::optional<std::string> date) {
  data_.date = date;
}

void meta_tool::set_photographer(std::optional<std::string> photographer) {
  data_.photographer = photographer;
}

void meta_tool::set_world(std::optional<std::string> world) {
  data_.world = world;
}

void meta_tool::add_user(std::string user) {
  // twitterのidがあれば分割
  const std::string_view delemiter = " : ";
  auto pos                         = user.rfind(delemiter);
  if (pos != std::string::npos) {
    data_.users.insert(
        std::make_pair(user.substr(0, pos), user.substr(pos + delemiter.length())));
  } else {
    data_.users.insert(std::make_pair(user, std::nullopt));
  }
}

void meta_tool::delete_user(std::string user_name) {
  data_.users.erase(user_name);
}

void meta_tool::clear_users() {
  std::map<std::string, std::optional<std::string>>().swap(data_.users);
}

void meta_tool::read(filesystem::path path) {
  // dateの初期化
  set_date(std::nullopt);
  set_photographer(std::nullopt);
  set_world(std::nullopt);
  clear_users();

  chunk_util util(path);
  try {
    auto chunks = util.read();
    for (auto chunk : *chunks) {
      auto [type, data] = chunk;
      if (type == "vrCd") {
        set_date(data);
      } else if (type == "vrCp") {
        set_photographer(data);
      } else if (type == "vrCw") {
        set_world(data);
      } else if (type == "vrCu") {
        add_user(data);
      }
    }
  } catch (...) {
    std::cout << "read png exception" << std::endl;
    return;
  }
}
} // namespace vrc_photo_album2::meta_tool
