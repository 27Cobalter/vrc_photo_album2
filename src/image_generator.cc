#include "image_generator.h"

#include <algorithm>
#include <iostream>

// #include <omp.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace vrc_photo_album2 {
image_generator::image_generator(const cv::Size output_size) : output_size_(output_size) {
  freetype2_ = cv::freetype::createFreeType2();
  freetype2_->loadFontData(font_, 0);
  // font_size_             = tmp_size * text_size / tmp_size;
  // user_font_size_        = font_size_ >> 1;
  // date_pos_              = cv::Point(0, output_size_.height * picture_ratio_);
  // world_pos_             = cv::Point(0, output_size_.height * picture_ratio_ + font_size_);
  // user_pos_              = cv::Point(output_size_.width * picture_ratio_, 0);
}

void image_generator::generate_single(const filesystem::path& path, cv::Mat& dst) {
  cv::Mat src = cv::imread(path);

  dst = cv::Mat::zeros(output_size_, CV_8UC3);
  meta_tool::meta_tool metadata;
  metadata.read(path);

  double scale = (static_cast<double>(dst.rows) / src.rows);
  if (metadata.has_any()) {
    scale *= picture_ratio_;
    // #pragma omp critical
    put_metadata(metadata, dst);
  } else if (src.size() == dst.size()) {
    // メタデータなしで同じサイズの画像
    dst = src.clone();
    return;
  }

  cv::Mat affine = (cv::Mat_<double>(2, 3) << scale, 0, 0, 0, scale, 0);
  cv::warpAffine(src, dst, affine, dst.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
}

void image_generator::generate_tile(const std::set<filesystem::path>::iterator path_begin,
                                    const std::set<filesystem::path>::iterator path_end,
                                    cv::Mat& dst) {
  std::vector<cv::Mat> images(
      std::min(static_cast<int>(std::distance(path_begin, path_end)), 9));
  dst = cv::Mat::zeros(output_size_, CV_8UC3);

  const int tile_width = 3;
  const int dx         = output_size_.width / tile_width;
  const int dy         = output_size_.height / tile_width;

  auto path_it = path_begin;
  for (auto image_it = images.begin(); image_it != images.end(); image_it++, path_it++) {
    // ディスクアクセスまとめたほうが早そう（測ってないけど）
    *image_it = cv::imread(*path_it);
  }
  int i = 8;
  for (auto image_it = images.begin(); image_it != images.end(); image_it++, i--) {
    const double scale = (static_cast<double>(dst.rows) / image_it->rows) / tile_width;
    cv::Mat affine = (cv::Mat_<double>(2, 3) << scale, 0, dx * std::floor(i % tile_width), 0,
                      scale, dy * (i / tile_width));
    cv::warpAffine(*image_it, dst, affine, dst.size(), cv::INTER_LINEAR,
                   cv::BORDER_TRANSPARENT);
  }
}

void image_generator::put_metadata(meta_tool::meta_tool& metadata, cv::Mat& dst) {
  const double tmp_size     = output_size_.height * ((1 - picture_ratio_) / 2);
  const double text_size    = freetype2_->getTextSize("y()|", tmp_size, thickness_, 0).height;
  const int font_size       = tmp_size * text_size / tmp_size;
  const int user_font_size  = font_size >> 1;
  const cv::Point date_pos  = cv::Point(0, output_size_.height * picture_ratio_);
  const cv::Point world_pos = cv::Point(0, output_size_.height * picture_ratio_ + font_size);
  const cv::Point user_pos  = cv::Point(output_size_.width * picture_ratio_, 0);

  if (metadata.has_date()) {
    freetype2_->putText(dst, metadata.readable_date(), date_pos, font_size, text_color_,
                        thickness_, cv::LINE_AA, false);
  }
  if (metadata.has_world()) {
    freetype2_->putText(dst, metadata.world(), world_pos, font_size, text_color_, thickness_,
                        cv::LINE_AA, false);
  }
  if (metadata.has_users()) {
    int i = 0;
    for (auto [user_name, screen_name] : metadata.users()) {
      freetype2_->putText(dst, user_name, user_pos + cv::Point(0, user_font_size * i++),
                          user_font_size, text_color_, thickness_, cv::LINE_AA, false);
    }
  }
}
} // namespace vrc_photo_album2