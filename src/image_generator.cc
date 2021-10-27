#include "image_generator.h"

#include <algorithm>
#include <iostream>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "util.h"

namespace vrc_photo_album2 {
image_generator::image_generator(const cv::Size output_size) : output_size_(output_size) {
  freetype2_ = cv::freetype::createFreeType2();
  freetype2_->loadFontData(font_, 0);
  tmp_size_       = output_size_.height * ((1 - picture_ratio_) / 2);
  text_size_      = freetype2_->getTextSize("y()|", tmp_size_, thickness_, 0).height;
  font_size_      = tmp_size_ * text_size_ / tmp_size_;
  user_font_size_ = font_size_ >> 1;
}

void image_generator::generate_single(const filesystem::path& path, const cv::Mat& src,
                                      cv::Mat& dst) {
  dst = cv::Mat::zeros(output_size_, CV_8UC3);
  meta_tool::meta_tool metadata;
  metadata.read(path);

  double scale = (static_cast<double>(dst.rows) / src.rows);
  int dx       = (dst.cols - src.cols * scale) / 2;
  if (metadata.has_any()) {
    scale *= picture_ratio_;
    dx *= picture_ratio_;
    put_metadata(metadata, dst);
  } else if (src.size() == dst.size()) {
    // メタデータなしで同じサイズの画像
    dst = src.clone();
    return;
  }

  cv::Mat affine = (cv::Mat_<double>(2, 3) << scale, 0, dx, 0, scale, 0);
  cv::warpAffine(src, dst, affine, dst.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
}

void image_generator::generate_tile(const std::set<filesystem::path>::iterator path,
                                    const std::vector<cv::Mat>& images, cv::Mat& dst) {
  dst = cv::Mat::zeros(output_size_, CV_8UC3);

  const int tile_width = 3;
  const int dx         = output_size_.width / tile_width;
  const int dy         = output_size_.height / tile_width;

  int i        = 0;
  auto path_it = path;
  for (auto image_it = images.begin(); image_it != images.end(); image_it++, i++, path_it++) {
    const double scale = (static_cast<double>(dst.rows) / image_it->rows) / tile_width;
    const int dx_tmp   = (dx - image_it->cols * scale) / 2;
    const int mx       = dx * std::floor(i % tile_width) + dx_tmp;
    const int my       = dy * (i / tile_width);
    cv::Mat affine     = (cv::Mat_<double>(2, 3) << scale, 0, mx, 0, scale, my);
    cv::warpAffine(*image_it, dst, affine, dst.size(), cv::INTER_LINEAR,
                   cv::BORDER_TRANSPARENT);
    const cv::Point date_pos =
        cv::Point(mx, my + (output_size_.height * (picture_ratio_ + 0.1)) / 3);
    std::string filename = filename_date(*path_it);
    filename[10]         = ' ';
    filename[13]         = ':';
    filename[16]         = ':';
    freetype2_->putText(dst, filename.substr(0, filename.size() - 4), date_pos, font_size_ / 3,
                        text_color_, 1, cv::LINE_AA, false);
  }
}

void image_generator::put_metadata(meta_tool::meta_tool& metadata, cv::Mat& dst) {
  const cv::Point date_pos  = cv::Point(0, output_size_.height * picture_ratio_);
  const cv::Point world_pos = cv::Point(0, output_size_.height * picture_ratio_ + font_size_);
  const cv::Point user_pos  = cv::Point(output_size_.width * picture_ratio_, 0);

  if (metadata.has_date()) {
    freetype2_->putText(dst, metadata.readable_date(), date_pos, font_size_, text_color_,
                        thickness_, cv::LINE_AA, false);
  }
  if (metadata.has_world()) {
    freetype2_->putText(dst, metadata.world(), world_pos, font_size_, text_color_, thickness_,
                        cv::LINE_AA, false);
  }
  if (metadata.has_users()) {
    int i = 0;
    for (auto [user_name, screen_name] : metadata.users()) {
      freetype2_->putText(dst, user_name, user_pos + cv::Point(0, user_font_size_ * i++),
                          user_font_size_, text_color_, thickness_, cv::LINE_AA, false);
    }
  }
}
} // namespace vrc_photo_album2
