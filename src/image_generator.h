#ifndef VRC_PHOTO_ALBUM2_IMAGE_GENERATOR_H_
#define VRC_PHOTO_ALBUM2_IMAGE_GENERATOR_H_

#include <filesystem>
#include <set>

#include <opencv2/core/core.hpp>
#include <opencv2/freetype.hpp>

#include "vrc_meta_tool.h"

namespace vrc_photo_album2 {
namespace filesystem = std::filesystem;

class image_generator {
public:
  image_generator(const cv::Size output_size);
  void generate_single(const filesystem::path& path, const cv::Mat& src, cv::Mat& dst);
  void generate_tile(const std::vector<cv::Mat>& images, cv::Mat& dst);

private:
  const char* font_            = "/usr/share/fonts/TTF/migu-1c-regular.ttf";
  const double picture_ratio_  = 0.8;
  const cv::Scalar text_color_ = {255, 255, 0};
  const int thickness_         = 2;
  cv::Ptr<cv::freetype::FreeType2> freetype2_;
  cv::Size output_size_;
  // int font_size_;
  // int user_font_size_;
  // cv::Point date_pos_;
  // cv::Point world_pos_;
  // cv::Point user_pos_;

  void put_metadata(meta_tool::meta_tool& metadata, cv::Mat& dst);
};
} // namespace vrc_photo_album2
#endif // VRC_PHOTO_ALBUM2_IMAGE_GENERATOR_H_
