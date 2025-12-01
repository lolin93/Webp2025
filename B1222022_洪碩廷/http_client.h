// http_client.h
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "url.h"
#include <string>
#include <chrono>

struct DownloadStats {
    std::size_t total_files = 0;
    std::size_t total_bytes = 0;
    std::chrono::steady_clock::time_point start_time;
};

// 【新增宣告】：讓其他檔案能使用這兩個輔助函式
bool ensure_parent_dir(const std::string& filename);
std::string fmt_size(std::size_t bytes);


// 單純下載一個 URL → 檔案
// 會在終端機印出：進度、速度、ETA
bool http_download_to_file(const Url& url,
                           const std::string& output_path,
                           DownloadStats* global_stats);

#endif // HTTP_CLIENT_H