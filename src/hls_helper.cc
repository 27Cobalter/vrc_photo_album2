#include <iostream>
#include "hls_helper.h"

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
      segment_.start = "";
      segment_.end   = "";
      segment_.video = "";
      return false;
    }
    if (line.size() > src_start_.size() && line.substr(0, src_start_.size()) == src_start_) {
      segment_.start = filesystem::path(line.substr(src_start_.size()));
    }
    if (line.size() > src_end_.size() && line.substr(0, src_end_.size()) == src_end_) {
      segment_.end = filesystem::path(line.substr(src_end_.size()));
    }
    if (line[0] != '#' && line.size() > file_extension_.size() &&
        line.substr(line.size() - file_extension_.size()) == file_extension_) {
      segment_.video = line;
      return true;
    }
  }
}

filesystem::path hls_manager::segment_start() const {
  return segment_.start.filename();
}

filesystem::path hls_manager::segment_end() const {
  return segment_.end.filename();
}

int hls_manager::segment_id() const {
  const std::string suffix_head = "video_";
  return std::atoi(segment_.video.substr(suffix_head.size(), 5).c_str());
}

bool hls_manager::compare_start(const filesystem::path path) const {
  return path.filename() == segment_.start.filename();
}

bool hls_manager::compare_end(const filesystem::path path) const {
  return path.filename() == segment_.end.filename();
}

void hls_manager::ifs_close() {
  ifs_->close();
}
} // namespace vrc_photo_album2
