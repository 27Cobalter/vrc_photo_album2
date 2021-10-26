#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

#include <boost/format.hpp>
#include <omp.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "hls_helper.h"
#include "image_generator.h"
#include "vrc_meta_tool.h"

namespace filesystem = std::filesystem;
using namespace vrc_photo_album2;

template <typename Iterator>
inline int n_or_end(Iterator it, Iterator end, int n) {
  return std::min(static_cast<int>(std::distance(it, end)), n);
}

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
  const int tile_size = 9;

  // パス取得部分
  for (const filesystem::directory_entry& x : filesystem::directory_iterator(input_dir)) {
    std::string path(x.path());
    if (path.substr(path.length() - 3) == "png") {
      resource_paths.insert(x.path());
    }
  }

  // 重複チェック
  hls_manager manager(video_dir.string() + "vrc_photo_album.m3u8");
  int update_index = 0;
  auto it          = resource_paths.begin();
  auto segment_num = (resource_paths.size() + tile_size - 1) / tile_size;
  for (; update_index < segment_num;) {
    if (!manager.next_segment()) {
      break;
    }
    auto end = std::next(it, n_or_end(it, resource_paths.end(), tile_size) - 1);
    if (manager.compare_start(it->filename()) && manager.compare_end(end->filename())) {
      std::cout << update_index << ". " << it->filename() << " - " << end->filename()
                << " is not changed." << std::endl;
      update_index++;
      it = std::next(end, 1);
    } else {
      std::cout << update_index << ". " << it->filename() << " - " << end->filename()
                << " is changed." << std::endl;
      break;
    }
  }
  std::cout << "update from " << update_index << " th block" << std::endl;

  // 画像生成部分
  // ここを消すと内側が並列化されるので1ブロック生成時は消すと良い（いい方法ない？）
#pragma omp parallel for
  for (int i = update_index; i < segment_num; i++) {
    const int index = i * tile_size;
    auto it         = std::next(resource_paths.begin(), index);
    std::vector<cv::Mat> images(n_or_end(it, resource_paths.end(), tile_size));
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
      image_generator generator(output_size);
      generator.generate_single(*id, images[j], dsts[j + 1]);
      // std::string log(std::string("") + "generate: " + output_dir.string() +
      //                 id->filename().string() + " -> " + output_dir.string() +
      //                 (boost::format("img-%05d_%05d.png") % i % (j + 1)).str());
      // std::cout << log << std::endl;
    }

#pragma omp parallel for
    for (int j = 0; j < dsts.size(); j++) {
      cv::imwrite(output_dir.string() + (boost::format("img-%05d_%05d.png") % i % (j)).str(),
                  dsts[j]);
      dsts[j].release();
    }

    // 10枚毎のブロック生成部分
    std::string command = (boost::format(std::string("") +
                                         "ffmpeg -loglevel error -framerate 2 "
                                         "-i " +
                                         output_dir.string() +
                                         "img-%05d_%s.png -vcodec libx264 "
                                         "-pix_fmt yuv420p -r 2 -f hls -hls_time 5 "
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
         "#EXT-X-TARGETDURATION:5\n"
         "#EXT-X-MEDIA-SEQUENCE:0\n"
         "#EXT-X-PLAYLIST-TYPE:EVENT\n\n";
  auto path = resource_paths.begin();
  for (int i = 0; i < segment_num; i++, std::advance(path, tile_size)) {
    auto end = std::next(path, n_or_end(path, resource_paths.end(), tile_size) - 1);
    ofs << boost::format("#src_start=%s") % path->filename().string() << "\n"
        << boost::format("#src_end=%s") % end->filename().string() << "\n"
        << "#EXT-X-DISCONTINUITY\n"
        << "#EXTINF:5.000000\n"
        << boost::format("video-%05d_00000.ts\n") % i;
  }
  ofs << "#EXT-X-ENDLIST";
  std::cout << "complete!" << std::endl;

  return 0;
}
