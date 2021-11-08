#include <cstdio>
#include <chrono>
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
#include "util.h"
#include "vrc_meta_tool.h"

namespace filesystem = std::filesystem;
using namespace vrc_photo_album2;

auto main(int argc, char** argv) -> int {
  cv::CommandLineParser parser(
      argc, argv,
      "{input|./resources|input directory}"
      "{output|./export|output directory. required sub folter output_dir/(png,video)/}"
      "{font|/usr/share/fonts/TTF/migu-1c-regular.ttf|font path}");

  const cv::Size output_size(1920, 1080);
  const filesystem::path font_path(parser.get<std::string>("font"));
  const filesystem::path input_dir(parser.get<std::string>("input"));
  const filesystem::path out_dir(parser.get<std::string>("output"));
  const filesystem::path output_dir(out_dir.string() + "/" + filesystem::path("png/").string());
  const filesystem::path video_dir(out_dir.string() + "/" +
                                   filesystem::path("video/").string());
  std::cout << "inputdir: " << input_dir << ", output_dir: " << out_dir
            << ", font: " << font_path.c_str() << std::endl;
  const filesystem::path tmp_dir(filesystem::temp_directory_path().string() +
                                 "/vrc_photo_album/");
  const filesystem::path m3u8_name("vrc_photo_album.m3u8");

  filesystem::path video_file = video_dir.string() + m3u8_name.string();
  filesystem::path tmp_file   = tmp_dir.string() + m3u8_name.string();

  auto input_time = filesystem::last_write_time(input_dir);
  auto input_tm   = conv_fclock(input_time);

  filesystem::create_directory(tmp_dir);

  // tmpファイルの存在確認
  filesystem::path* read_meta_file = &video_file;
  if (filesystem::exists(tmp_file)) {
    std::cout << "tmp m3u8 found" << std::endl;
    read_meta_file = &tmp_file;
  } else {
    std::cout << "tmp m3u8 not found" << std::endl;
    filesystem::copy_file(video_file, tmp_file, filesystem::copy_options::update_existing);
    filesystem::last_write_time(tmp_file, input_time);
  }

  // m3u8が存在したら更新日時を確認
  if (filesystem::exists(*read_meta_file)) {
    auto checker_time = filesystem::last_write_time(*read_meta_file);
    auto checker_tm   = conv_fclock(checker_time);
    std::cout << "inputdir time: " << std::put_time(&input_tm, "%c")
              << " checker time: " << std::put_time(&checker_tm, "%c") << std::endl;
    if (input_time == checker_time) {
      std::cout << "inputdir not changed!" << std::endl;
      return 0;
    } else {
      std::cout << "inputdir changed!" << std::endl;
    }
  }

  std::vector<std::filesystem::path> resource_paths;
  const int tile_size = 9;

  // パス取得部分
  auto start = std::chrono::system_clock::now();
  for (const filesystem::directory_entry& x : filesystem::directory_iterator(input_dir)) {
    std::string path(x.path());
    if (path.substr(path.length() - 3) == "png") {
      resource_paths.push_back(x.path());
    }
  }
  std::sort(resource_paths.begin(), resource_paths.end());
  auto end = std::chrono::system_clock::now();
  std::cout << "get paths time:"
            << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
            << std::endl;

  // 重複チェック
  start = std::chrono::system_clock::now();
  hls_manager manager(*read_meta_file);
  int update_index = 0;
  auto it          = resource_paths.begin();
  int segment_num  = (resource_paths.size() + tile_size - 1) / tile_size;
  for (; update_index < segment_num;) {
    if (!manager.next_segment()) {
      break;
    }
    auto end = std::next(it, bound_load(it, resource_paths.end(), tile_size) - 1);
    if (manager.compare_start(it->filename()) && manager.compare_end(end->filename())) {
      // std::cout << update_index << ". " << it->filename() << " - " << end->filename()
      //           << " is not changed." << std::endl;
      update_index++;
      it = std::next(end, 1);
    } else {
      std::cout << update_index << ". " << it->filename() << " - " << end->filename()
                << " is changed." << std::endl;
      break;
    }
  }
  end = std::chrono::system_clock::now();
  std::cout << "check update time:"
            << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
            << std::endl;

  // ファイルの更新なしの場合
  if (update_index >= segment_num) {
    std::cout << "file not changed" << std::endl;
    filesystem::last_write_time(video_file, input_time);
    filesystem::last_write_time(tmp_file, input_time);
    return 0;
  }

  std::cout << "update from " << update_index << " th block" << std::endl;

  // fontをtmpfsへコピー
  const filesystem::path tmp_font = tmp_dir.string() + font_path.filename().string();
  std::cout << "copy" << font_path << " -> " << tmp_font << std::endl;
  filesystem::copy_file(font_path, tmp_font, filesystem::copy_options::update_existing);

  const filesystem::path blank_path = "./blank.png";
  const cv::Mat blank_image         = cv::imread(blank_path);

// 画像生成部分
// ここを消すと内側が並列化されるので1ブロック生成時は消すと良い（いい方法ない？）
#pragma omp parallel for
  for (int i = update_index; i < segment_num; i++) {
    const int index = i * tile_size;
    auto it         = std::next(resource_paths.begin(), index);
    int bound       = bound_load(it, resource_paths.end(), tile_size);
    std::vector<cv::Mat> images(bound);
    std::vector<cv::Mat> dsts(tile_size + 1);
#pragma omp parallel for
    for (int j = 0; j < bound; j++) {
      images[j] = cv::imread(*(std::next(it, j)));
    }
    image_generator generator(output_size, tmp_font);
    generator.generate_tile(it, images, dsts[0]);

#pragma omp parallel for
    for (int j = 0; j < bound; j++) {
      auto id = std::next(it, j);
      image_generator generator(output_size, tmp_font);
      generator.generate_single(*id, images[j], dsts[j + 1]);
      // std::string log(std::string("") + "generate: " + output_dir.string() +
      //                 id->filename().string() + " -> " + output_dir.string() +
      //                 (boost::format("img-%05d_%05d.png") % i % (j + 1)).str());
      // std::cout << log << std::endl;
    }
    for (int j = bound + 1; j < dsts.size(); j++) {
      dsts[j] = blank_image;
    }

#pragma omp parallel for
    for (int j = 0; j < dsts.size(); j++) {
      cv::imwrite(output_dir.string() + (boost::format("img-%06d_%05d.png") % i % (j)).str(),
                  dsts[j]);
      dsts[j].release();
    }

    // 10枚毎のブロック生成部分
    std::string command = (boost::format("ffmpeg -loglevel error -framerate 1 "
                                         "-i %simg-%06d_%s.png -vcodec libx264 "
                                         "-pix_fmt yuv420p -r 5 -f hls -hls_time 10 "
                                         "-hls_playlist_type vod -hls_segment_filename "
                                         "\"%s%06d%s.ts\" %s%06d.m3u8") %
                           output_dir.string() % i % "%05d" % video_dir.string() % i % "%1d" %
                           video_dir.string() % i)
                              .str();
    std::cout << command << std::endl;
    std::system(command.c_str());
  }

  // hlsのメタデータ変更部分
  std::cout << "writeing m3u8" << std::endl;
  const std::string m3head = {
      "#EXTM3U\n"
      "#EXT-X-VERSION:3\n"
      "#EXT-X-TARGETDURATION:10\n"
      "#EXT-X-MEDIA-SEQUENCE:0\n"
      "#EXT-X-PLAYLIST-TYPE:EVENT\n\n"};
  std::stringstream m3stream;
  std::stringstream m3index;
  auto path = resource_paths.begin();
  for (int i = 0; i < segment_num; i++, std::advance(path, tile_size)) {
    auto end = std::next(path, bound_load(path, resource_paths.end(), tile_size) - 1);
    m3index << boost::format("#v%06d,%s,%s\n") % i % filename_date(*path) % filename_date(*end);
    m3stream << boost::format(
                    "#EXT-X-DISCONTINUITY\n"
                    "#EXTINF:10\n"
                    "%06d%01d.ts\n") %
                    (segment_num - i - 1) % 0;
  }
  m3stream << "#EXT-X-ENDLIST\n";

  // 実体書き込み
  start = std::chrono::system_clock::now();
  std::ofstream ofs(video_file);
  ofs << m3head << m3index.str() << m3stream.str();
  ofs.close();

  // tmpファイル書き込み
  ofs = std::ofstream(tmp_file);
  ofs << m3index.str();
  end = std::chrono::system_clock::now();
  std::cout << "file write time:"
            << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
            << std::endl;
  ofs.close();

  filesystem::last_write_time(video_file, input_time);
  filesystem::last_write_time(tmp_file, input_time);

  std::cout << "complete!" << std::endl;

  return 0;
}
