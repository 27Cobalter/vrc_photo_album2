#ifndef VRC_PHOTO_ALBUM2_UTIL_H
#define VRC_PHOTO_ALBUM2_UTIL_H

#include <string>
#include <filesystem>

namespace vrc_photo_album2 {
namespace filesystem = std::filesystem;
inline std::string filename_date(filesystem::path path) {
  // VRChat_CCCCxRRRR_YYYY-mm-dd_HH_MM-ss.SSS.pngを想定
  return path.filename().string().substr(17, 23);
}

template <typename Iterator>
inline int n_or_end(Iterator it, Iterator end, int n) {
  return std::min(static_cast<int>(std::distance(it, end)), n);
}
} // namespace vrc_photo_album2
#endif
