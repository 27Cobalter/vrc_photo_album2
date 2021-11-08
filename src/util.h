#ifndef VRC_PHOTO_ALBUM2_UTIL_H
#define VRC_PHOTO_ALBUM2_UTIL_H

#include <chrono>
#include <filesystem>
#include <string>

namespace vrc_photo_album2 {
namespace filesystem = std::filesystem;
inline std::string filename_date(filesystem::path path) {
  // VRChat_CCCCxRRRR_YYYY-mm-dd_HH_MM-ss.SSS.pngを想定
  return path.filename().string().substr(17, 23);
}

template <typename Iterator>
inline int bound_load(Iterator it, Iterator end, int n) {
  return std::min(static_cast<int>(std::distance(it, end)), n);
}

inline std::tm conv_fclock(filesystem::file_time_type ftime) {
  namespace chrono  = std::chrono;
  std::time_t count = chrono::duration_cast<chrono::seconds>(
                          chrono::file_clock::to_sys(ftime).time_since_epoch())
                          .count();
  const std::tm* lt = std::localtime(&count);
  return *lt;
}
} // namespace vrc_photo_album2
#endif
