#include <iostream>

#include "hls_helper.h"
#include "util.h"

namespace vrc_photo_album2 {

hls_manager::hls_manager(const filesystem::path path) : path_(path) {
  ifs_ = std::make_unique<std::ifstream>(path_);
}

hls_manager::~hls_manager() {
  ifs_->close();
}

bool hls_manager::next_segment() {
  std::string line;
  for (;;) {
    if (!std::getline(*ifs_, line)) {
      segment_.index = "";
      segment_.start = "";
      segment_.end   = "";
      return false;
    }
    auto index_pos = m3index_tag.size();
    if (line.size() > index_pos && line.substr(0, index_pos) != m3index_tag) {
      continue;
    }
    auto start_pos = line.find(",", index_pos);
    auto end_pos   = line.find(",", start_pos + 1);
    if (start_pos != std::string::npos && end_pos != std::string::npos) {
      segment_.index = line.substr(index_pos, start_pos - index_pos);
      segment_.start = line.substr(start_pos + 1, end_pos - start_pos - 1);
      segment_.end   = line.substr(end_pos + 1);
      return true;
    }
  }
}

std::string hls_manager::segment_start() const {
  return segment_.start;
}

std::string hls_manager::segment_end() const {
  return segment_.end;
}

int hls_manager::segment_id() const {
  return std::atoi(segment_.index.c_str());
}

bool hls_manager::compare_start(const filesystem::path path) const {
  return filename_date(path.filename()) == segment_.start;
}

bool hls_manager::compare_end(const filesystem::path path) const {
  return filename_date(path) == segment_.end;
}

void hls_manager::ifs_close() {
  ifs_->close();
}
} // namespace vrc_photo_album2
