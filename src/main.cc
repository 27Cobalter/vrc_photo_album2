#include "vrc_meta_tool.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

#include <boost/format.hpp>
#include <omp.h>
#include <opencv2/core/core.hpp>
#include <opencv2/freetype.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "image_generator.h"

namespace filesystem = std::filesystem;
using namespace vrc_photo_album2;

auto main(int argc, char** argv) -> int {
  cv::CommandLineParser parser(
      argc, argv,
      "{input|./resources|input directory}"
      "{output|./export|output directory. required sub folter output_dir/(png,video)/}");

  const cv::Size output_size(1920, 1080);
  const filesystem::path input_dir = parser.get<std::string>("input");
  const filesystem::path out_dir   = filesystem::path(parser.get<std::string>("output"));
  const filesystem::path output_dir =
      out_dir.string() + "/" + filesystem::path("png/").string();
  const filesystem::path video_dir =
      out_dir.string() + "/" + filesystem::path("video/").string();
  std::cout << "inputdir: " << input_dir << ", output_dir" << out_dir << std::endl;

  std::set<std::filesystem::path> resource_paths;
  // メモリリソース削減するならfreetypeを共有化する
  // image_generator generator(output_size);

  // パス取得部分
  for (const filesystem::directory_entry& x : filesystem::directory_iterator(input_dir)) {
    std::string path(x.path());
    if (path.substr(path.length() - 3) == "png") {
      resource_paths.insert(x.path());
    }
  }

  // 画像生成部分
  const int tile_size = 9;
// #pragma omp parallel for num_threads(1)
#pragma omp parallel for
  for (int i = 0; i < (resource_paths.size() + tile_size - 1) / tile_size; i++) {
    const int index = i * tile_size;
    auto it         = std::next(resource_paths.begin(), index);
    std::vector<cv::Mat> images(
        std::min(static_cast<int>(std::distance(it, resource_paths.end())), tile_size));
    std::vector<cv::Mat> dsts(images.size() + 1);
#pragma omp parallel for
    for (int j = 0; j < images.size(); j++) {
      images[j] = cv::imread(*(std::next(it, j)));
    }
    image_generator generator(output_size);
    generator.generate_tile(images, dsts[0]);

#pragma omp parallel for
    for (int j = 0; j < images.size(); j++) {
      auto id = std::next(it, j);
      // std::cout << id->filename().string() << std::endl;
      image_generator generator(output_size);
      generator.generate_single(*id, images[j], dsts[j + 1]);
      std::string log(std::string("") + "generate: " + output_dir.string() +
                      id->filename().string() + " -> " + output_dir.string() +
                      (boost::format("img-%05d_%05d.png") % i % (j + 1)).str());
#pragma omp critical
      std::cout << log << std::endl;
    }

#pragma omp parallel for
    for (int j = 0; j < dsts.size(); j++) {
      auto id = std::next(it, j);
      cv::imwrite(output_dir.string() + (boost::format("img-%05d_%05d.png") % i % (j)).str(),
                  dsts[j]);
      dsts[j].release();
    }
  }

  // 10枚毎のブロック生成部分
#pragma omp parallel for
  for (int i = 0; i < (resource_paths.size() + 8) / 9; i++) {
    std::string command = (boost::format(std::string("") +
                                         "ffmpeg -framerate 10 "
                                         "-i " +
                                         output_dir.string() +
                                         "img-%05d_%s.png -vcodec libx264 "
                                         "-pix_fmt yuv420p -r 10 -f hls -hls_time 1 "
                                         "-hls_playlist_type vod -hls_segment_filename "
                                         "\"" +
                                         video_dir.string() + "video-%05d_%s.ts\" " +
                                         video_dir.string() + "video-%05d.m3u8") %
                           i % "%05d" % i % "%05d" % i)
                              .str();
    std::cout << command << std::endl;
    std::system(command.c_str());
  }

  // hlsのメタデータ変更部分
  std::cout << "writeing m3u8" << std::endl;
  std::ofstream ofs(video_dir.string() + "vrc_photo_album.m3u8");
  ofs << "#EXTM3U\n"
         "#EXT-X-VERSION:3\n"
         "#EXT-X-TARGETDURATION:1\n"
         "#EXT-X-MEDIA-SEQUENCE:0\n"
         "#EXT-X-PLAYLIST-TYPE:EVENT\n\n";
  auto path = resource_paths.begin();
  for (int i = 0; i < (resource_paths.size() + 8) / 9; i++, path = std::next(path, 9)) {
    ofs << boost::format("#%s") % path->filename().string() << "\n"
        << "#EXT-X-DISCONTINUITY\n"
        << "#EXTINF:1.000000\n"
        << boost::format("video-%05d_00000.ts\n") % i;
  }
  ofs << "#EXT-X-ENDLIST";

  return 0;
}
