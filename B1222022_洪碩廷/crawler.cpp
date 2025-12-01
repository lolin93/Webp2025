// crawler.cpp
// 此版本包含了：自動遞迴爬取、深度限制、外站限制、連結重寫/相對路徑、二進位 I/O、續傳前檢查
#include "crawler.h"
#include <set>
#include <vector>
#include <queue>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm> // 為了 std::min
#include <iterator>  // 為了 std::istreambuf_iterator

// 檢查檔案是否存在
// 【修正】：移除 static
bool file_exists(const std::string& path) {
    return (access(path.c_str(), F_OK) == 0);
}

// 建立根目錄
// 【修正】：移除 static
void ensure_dir(const std::string& dir) {
    if (dir.empty()) return;
    struct stat st;
    if (stat(dir.c_str(), &st) != 0) {
        mkdir(dir.c_str(), 0755);
    }
}

// ----------------------------------------------------------------------
// 輔助函式：URL 轉成本機路徑
// ----------------------------------------------------------------------
std::string url_to_local_path(const Url& url, const std::string& output_dir) {
    std::string path = url.path;
    if (path.empty() || path == "/") {
        // 將根路徑存為 index.htm
        path = "/index.htm"; 
    }
    // 確保路徑以 / 開頭，並加上 output_dir
    if (path[0] != '/') path = "/" + path;
    return output_dir + path;
}

// ----------------------------------------------------------------------
// 輔助函式：計算相對路徑
// ----------------------------------------------------------------------
// 【修正】：移除 static
std::string relative_path(const std::string& current_local_path_rel,
                                 const std::string& target_local_path_rel) {
    std::string path1 = current_local_path_rel.substr(1);
    std::string path2 = target_local_path_rel.substr(1);

    auto split_path = [](const std::string& p) {
        std::vector<std::string> parts;
        if (p.empty()) return parts;
        std::size_t start = 0;
        std::size_t end;
        while ((end = p.find('/', start)) != std::string::npos) {
            if (end > start) parts.push_back(p.substr(start, end - start));
            start = end + 1;
        }
        if (start < p.size()) parts.push_back(p.substr(start));
        return parts;
    };
    
    std::string dir1 = path1;
    std::size_t slash = path1.rfind('/');
    if (slash != std::string::npos) dir1 = path1.substr(0, slash); 
    else dir1 = ""; 

    std::vector<std::string> parts1 = split_path(dir1);
    std::vector<std::string> parts2 = split_path(path2);

    int common = 0;
    for (size_t i = 0; i < parts1.size() && i < parts2.size(); ++i) {
        if (parts1[i] == parts2[i]) {
            common++;
        } else {
            break;
        }
    }

    std::string result = "";
    // 回退層數
    for (size_t i = common; i < parts1.size(); ++i) {
        result += "../";
    }

    // 加上目標路徑的剩餘部分
    for (size_t i = common; i < parts2.size(); ++i) {
        result += parts2[i];
        if (i < parts2.size() - 1) {
            result += "/";
        }
    }
    
    return result;
}


// ----------------------------------------------------------------------
// 連結擷取函式 (支援 HTML/CSS 連結)
// ----------------------------------------------------------------------
// 【修正】：移除 static
void extract_links(const std::string& content,
                          const Url& base_url,
                          bool allow_external,
                          std::vector<std::string>& out) {
    auto add_link = [&](const std::string& link) {
        if (link.empty() || link.front() == '#' || link.front() == '?') return; 
        std::string full = join_url(base_url, link);

        Url u;
        if (full.find("http://") != 0 || !parse_url(full, u)) return; 

        if (!allow_external && u.host != base_url.host) {
            return;
        }
        out.push_back(full);
    };

    std::size_t pos = 0;
    while (true) {
        std::size_t p = std::string::npos;
        char quote = '\0';
        std::string link_tag = "";

        // 查找常見的標籤格式 (href, src, url)
        std::size_t h_d = content.find("href=\"", pos); 
        std::size_t h_s = content.find("href='", pos); 
        std::size_t s_d = content.find("src=\"", pos);  
        std::size_t s_s = content.find("src='", pos);  
        std::size_t u_low = content.find("url(", pos); 

        // 找到最近的標籤開始
        p = std::min({
            h_d == std::string::npos ? content.size() : h_d,
            h_s == std::string::npos ? content.size() : h_s,
            s_d == std::string::npos ? content.size() : s_d,
            s_s == std::string::npos ? content.size() : s_s,
            u_low == std::string::npos ? content.size() : u_low
        });
        
        if (p == content.size()) break;

        // 確定是哪種標籤
        if (p == h_d) { link_tag = "href=\""; quote = '"'; }
        else if (p == h_s) { link_tag = "href='"; quote = '\''; }
        else if (p == s_d) { link_tag = "src=\""; quote = '"'; }
        else if (p == s_s) { link_tag = "src='"; quote = '\''; }
        else if (p == u_low) { link_tag = "url("; quote = '\0'; } 

        std::size_t start = p + link_tag.size();
        std::size_t end;
        
        // 處理 url(...)
        if (link_tag == "url(") {
            char search_quote = '\0';
            if (start < content.size() && (content[start] == '\'' || content[start] == '"')) {
                search_quote = content[start];
                start++;
            }
            if (search_quote != '\0') {
                end = content.find(search_quote, start);
            } else {
                end = content.find(')', start);
            }
            if (end == std::string::npos || end <= start) {
                pos = start;
                continue; 
            }
            // 清理 url() 尾部可能的多餘字元
            std::string link = content.substr(start, end - start);
            while (!link.empty() && (link.back() == ')' || link.back() == '\'' || link.back() == '"')) {
                link.pop_back();
            }
            add_link(link);
            pos = end + (search_quote != '\0' ? 1 : 0);
            
        } 
        // 處理 href/src
        else {
            end = content.find(quote, start);
            if (end == std::string::npos) {
                pos = start; 
                continue; 
            }
            std::string link = content.substr(start, end - start);
            add_link(link);
            pos = end + 1;
        }
    }
}

// ----------------------------------------------------------------------
// 連結重寫函式 (核心離線功能)
// ----------------------------------------------------------------------
// 【修正】：移除 static
std::string rewrite_content(const std::string& content,
                                   const Url& base_url,
                                   const std::string& current_local_path,
                                   const std::string& output_dir) {
    std::string rewritten_content = content;
    // 增加更多可能出現連結的 tag，例如 data-src, background-image 等，可以提高爬取率
    std::vector<std::string> tags = {"href=\"", "src=\"", "href='", "src='", "url("};

    for (const std::string& tag_prefix : tags) {
        std::size_t tag_pos = 0;
        
        while (true) {
            tag_pos = rewritten_content.find(tag_prefix, tag_pos);
            if (tag_pos == std::string::npos) break;
            
            bool is_quoted = (tag_prefix.back() == '"' || tag_prefix.back() == '\'');
            char original_quote = is_quoted ? tag_prefix.back() : '\0';

            std::size_t start = tag_pos + tag_prefix.size();
            std::size_t end;
            
            char search_quote = original_quote;
            if (tag_prefix == "url(") {
                if (start < rewritten_content.size() && (rewritten_content[start] == '\'' || rewritten_content[start] == '"')) {
                    search_quote = rewritten_content[start];
                    start++;
                }
            }
            
            if (search_quote != '\0') {
                end = rewritten_content.find(search_quote, start);
            } else {
                end = rewritten_content.find(')', start);
            }

            if (end == std::string::npos || end <= start) {
                tag_pos = start;
                continue;
            }

            std::string link = rewritten_content.substr(start, end - start);
            while (!link.empty() && (link.back() == ')' || link.back() == '\'' || link.back() == '"')) {
                link.pop_back();
            }

            // 1. 轉成絕對 URL
            std::string full_url_str = join_url(base_url, link);
            
            // 2. 解析 URL
            Url target_url;
            if (full_url_str.find("http://") != 0 || !parse_url(full_url_str, target_url) || target_url.host != base_url.host) {
                tag_pos = end + 1;
                continue; 
            }

            // 3. 得到目標本機路徑
            std::string target_local_path = url_to_local_path(target_url, output_dir);
            
            // 4. 計算相對路徑
            std::string current_path_rel = current_local_path.substr(output_dir.size());
            std::string target_path_rel = target_local_path.substr(output_dir.size());

            std::string new_link = relative_path(current_path_rel, target_path_rel); // 使用修正後的函式

            // 5. 替換
            std::size_t original_len = end - start;
            std::size_t rewritten_offset = start;
            
            // 替換
            rewritten_content.replace(rewritten_offset, original_len, new_link);

            // 更新下次查找位置
            std::ptrdiff_t length_diff = new_link.size() - original_len;
            tag_pos = end + length_diff + (search_quote != '\0' ? 1 : 0);
        }
    }

    return rewritten_content;
}


// ----------------------------------------------------------------------
// 爬蟲主邏輯
// ----------------------------------------------------------------------
void crawl_site(const Config& cfg) {
    Url start_url;
    if (!parse_url(cfg.url_str, start_url)) {
        // 此處不需處理 HTTPS，因為老師要求只測試 HTTP
        std::cerr << "URL 解析失敗: " << cfg.url_str << "\n";
        return;
    }

    if (!cfg.resume) {
        // -c 0: 重新下載，先刪除舊資料
        std::string cmd = "rm -rf '" + cfg.output_dir + "'";
        std::system(cmd.c_str());
    }
    ensure_dir(cfg.output_dir); // 使用修正後的函式

    struct Node {
        std::string url_str;
        int depth;
    };

    std::queue<Node> q;
    std::set<std::string> visited;

    // 自動模式：只加入起始 URL
    q.push({cfg.url_str, 0});
    visited.insert(cfg.url_str);

    DownloadStats global_stats;
    global_stats.start_time = std::chrono::steady_clock::now();

    while (!q.empty()) {
        Node cur = q.front();
        q.pop();

        Url u;
        if (!parse_url(cur.url_str, u)) continue;

        std::string local_path = url_to_local_path(u, cfg.output_dir);

        bool is_html = (local_path.size() >= 4 &&
                        (local_path.substr(local_path.size() - 4) == ".htm" ||
                         local_path.substr(local_path.size() - 5) == ".html"));
        bool is_css = (local_path.size() >= 4 && local_path.substr(local_path.size() - 4) == ".css");
        
        bool should_download = true;

        if (cfg.resume && file_exists(local_path)) { // 使用修正後的函式
            // -c 1: 續傳模式，跳過已存在檔案
            std::cout << "[跳過已存在] " << local_path << "\n";
            should_download = false;
        } 
        
        if (should_download) {
            std::cout << "\n[取回] depth=" << cur.depth << " " << cur.url_str
                      << " -> " << local_path << "\n";

            // http_download_to_file 應會自動建立父目錄
            bool ok = http_download_to_file(u, local_path, &global_stats);
            if (!ok) {
                std::cerr << "[下載失敗] " << cur.url_str << "\n";
                continue;
            }
        }
        
        // 若深度已達上限 (僅針對 HTML 頁面)，則不再解析
        if (cur.depth >= cfg.depth && is_html) continue;

        // 僅對 HTML 和 CSS 檔案進行解析/重寫
        if (!is_html && !is_css) continue; 


        // 讀檔，準備解析/重寫 (使用 std::ios::binary 確保讀取二進制資料完整)
        std::ifstream fin(local_path.c_str(), std::ios::binary); 
        if (!fin) {
            std::cerr << "[錯誤] 無法讀取檔案以進行解析/重寫: " << local_path << "\n";
            continue;
        }
        std::string content((std::istreambuf_iterator<char>(fin)),
                            std::istreambuf_iterator<char>());
        fin.close();

        // 核心：連結重寫 (實作離線瀏覽要求)
        std::string processed_content = rewrite_content(content, u, local_path, cfg.output_dir); // 使用修正後的函式

        // 重新寫回檔案 (使用 std::ios::binary 確保寫入二進制資料完整)
        std::ofstream fout(local_path.c_str(), std::ios::trunc | std::ios::binary); 
        if (fout) {
            fout.write(processed_content.data(), processed_content.size());
            fout.close();
        } else {
            std::cerr << "[警告] 無法重新寫入檔案以重寫連結: " << local_path << "\n";
        }

        // 解析連結
        std::vector<std::string> links;
        extract_links(processed_content, u, cfg.allow_external, links); // 使用修正後的函式

        for (const std::string& link_url : links) {
            if (visited.insert(link_url).second) {
                // HTML 深度+1，CSS 深度不變
                int next_depth = is_html ? cur.depth + 1 : cur.depth;
                if (next_depth <= cfg.depth) {
                    q.push({link_url, next_depth});
                }
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    // 【修正】：將 global_stats->total_files 改為 global_stats.total_files (因為它是物件)
    double elapsed = std::chrono::duration<double>(end - global_stats.start_time).count();

    std::cout << "\n=== 下載統計資訊 ===\n";
    std::cout << " 已下載檔案數量 : " << global_stats.total_files << "\n"; // 使用 .
    std::cout << " 已下載總容量   : " << global_stats.total_bytes << " bytes\n"; // 使用 .
    std::cout << " 已下載時間     : " << elapsed << " 秒\n";
    std::cout << "====================\n";
}