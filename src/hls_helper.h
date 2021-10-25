#ifndef VRC_PHOTO_ALBUM2_HLS_HELPER_H
#define VRC_PHOTO_ALBUM2_HLS_HELPER_H

#include <filesystem>
#include <memory>
#include <fstream>
#include <string>

namespace vrc_photo_album2 {
namespace filesystem = std::filesystem;
struct hls_segment {
  filesystem::path start;
  filesystem::path end;
  std::string video;
};
class hls_manager {
public:
  hls_manager(const filesystem::path path);
  ~hls_manager();
  bool next_segment();
  filesystem::path segment_start() const;
  filesystem::path segment_end() const;
  int segment_id() const;
  bool compare_start(const filesystem::path path) const;
  bool compare_end(const filesystem::path path) const;
  void ifs_close();

private:
  filesystem::path path_;
  hls_segment segment_;
  std::shared_ptr<std::ifstream> ifs_;
  const std::string src_start_      = "#src_start=";
  const std::string src_end_        = "#src_end=";
  const std::string file_extension_ = ".ts";
};
} // namespace vrc_photo_album2

#endif
