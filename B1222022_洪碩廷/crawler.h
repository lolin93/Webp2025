// crawler.h
#ifndef CRAWLER_H
#define CRAWLER_H

#include "config.h"
#include "http_client.h"
#include "url.h"
#include <string>
#include <vector> // 確保 vector 可用

// 【新增宣告】：讓其他檔案能使用這些輔助函式
bool file_exists(const std::string& path);
void ensure_dir(const std::string& dir);
std::string relative_path(const std::string& current_local_path_rel,
                          const std::string& target_local_path_rel);
void extract_links(const std::string& content,
                          const Url& base_url,
                          bool allow_external,
                          std::vector<std::string>& out);
std::string rewrite_content(const std::string& content,
                                   const Url& base_url,
                                   const std::string& current_local_path,
                                   const std::string& output_dir);


// 新增：宣告一個用於計算 URL 存到本機後路徑的輔助函式
std::string url_to_local_path(const Url& url, const std::string& output_dir);

void crawl_site(const Config& cfg);

#endif // CRAWLER_H