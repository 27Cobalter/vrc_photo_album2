#include "vrc_meta_tool.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include <boost/format.hpp>

namespace vrc_photo_album2::meta_tool {

chunk_util::chunk_util(filesystem::path path) : path_(path) {}

decltype(auto) chunk_util::parse_chunk(chunk_s& chunk) {
  // chunk.dataは終端文字が入っていないため部分文字列を取得する
  return std::make_tuple(std::string(reinterpret_cast<char*>(chunk.head_.type), 4),
                         std::string(reinterpret_cast<char*>(chunk.data_), chunk.size()));
}

decltype(auto) chunk_util::read() {
  constexpr int png_header_size                = 8;
  const unsigned char png_sig[png_header_size] = {0x89, 0x50, 0x4e, 0x47,
                                                  0x0d, 0x0a, 0x1a, 0x0a};
  auto meta_chunks = std::make_unique<std::vector<std::tuple<std::string, std::string>>>();

  char header[png_header_size];
  std::ifstream ifs(path_.string(), std::ios::binary);
  ifs.read(header, 8);

  if (std::strncmp((char*)header, (char*)png_sig, png_header_size) != 0) {
    throw std::invalid_argument("not png file.");
  }

  std::stringstream ss{
      std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>())};

  std::vector<const char*> vrc_meta_chunks;
  vrc_meta_chunks.push_back("vrCd");
  vrc_meta_chunks.push_back("vrCp");
  vrc_meta_chunks.push_back("vrCw");
  vrc_meta_chunks.push_back("vrCu");

  for (;;) {
    chunk_s ch;
    ss.read((char*)&ch.head_, sizeof(header));

    if (std::strncmp(ch.head_.type, "IEND", 4) == 0) {
      return meta_chunks;
    }

    bool skip = true;
    for (auto meta_chunk : vrc_meta_chunks) {
      if (std::strncmp(ch.head_.type, meta_chunk, 4) == 0) {
        skip = false;
        std::vector<char> dat(ch.size());
        ch.data_ = dat.data();
        ss.read((char*)ch.data_, ch.size());
        ss.seekg(4, std::iostream::cur);
        auto chunk = parse_chunk(ch);
        meta_chunks->push_back(chunk);
      }
    }

    if (skip) {
      ss.seekg(ch.size() + 4, std::iostream::cur);
    }
  }
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
    // auto chunks = util.read();
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
    std::cout << "read png exception: " << path.string() << std::endl;
  }
}
} // namespace vrc_photo_album2::meta_tool
