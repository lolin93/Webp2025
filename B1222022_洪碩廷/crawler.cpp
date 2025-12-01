// crawler.cpp
// 此版本包含了：自動遞迴爬取、深度限制、外站限制、連結重寫/相對路徑、二進位 I/O、續傳前檢查
#include "crawler.h" // 引入爬蟲相關的宣告 (例如 crawl_site 函式)
#include <set>       // 引入 std::set，用於儲存已訪問的 URL (避免重複爬取)
#include <vector>    // 引入 std::vector，用於儲存連結列表
#include <queue>     // 引入 std::queue，用於實現 BFS (廣度優先搜尋) 的工作佇列
#include <fstream>   // 引入檔案串流，用於讀取和寫入檔案內容
#include <iostream>  // 引入標準輸入/輸出 (例如 std::cout, std::cerr)
#include <sys/stat.h> // 引入 stat 相關的系統呼叫 (用於檢查檔案狀態/權限)
#include <unistd.h>  // 引入 POSIX API (例如 access 函式)
#include <cstdlib>   // 引入 C 標準函式庫 (例如 std::system)
#include <algorithm> // 為了 std::min (用於 extract_links 中找到最短的標籤開頭)
#include <iterator>  // 為了 std::istreambuf_iterator (用於高效地讀取整個檔案到字串)

// 檢查檔案是否存在
// 【修正】：移除 static (確保這個函式能在其他檔案被呼叫)
bool file_exists(const std::string& path) {
    // access() 函式檢查檔案是否存在。F_OK 表示檢查檔案存在性。
    return (access(path.c_str(), F_OK) == 0);
}

// 建立根目錄
// 【修正】：移除 static (確保這個函式能在其他檔案被呼叫)
void ensure_dir(const std::string& dir) {
    if (dir.empty()) return; // 如果路徑為空，則直接返回
    struct stat st;          // 宣告 stat 結構，用於儲存檔案/目錄資訊
    // stat() 嘗試取得目錄資訊。如果回傳非 0 (通常是 -1)，表示目錄不存在。
    if (stat(dir.c_str(), &st) != 0) {
        // 使用 mkdir 建立目錄，0755 是給予擁有者讀寫執行，其他人讀和執行權限
        mkdir(dir.c_str(), 0755);
    }
}

// ----------------------------------------------------------------------
// 輔助函式：URL 轉成本機路徑
// ----------------------------------------------------------------------
std::string url_to_local_path(const Url& url, const std::string& output_dir) {
    std::string path = url.path; // 取得 URL 的路徑部分 (例如 /about/index.html)
    if (path.empty() || path == "/") {
        // 如果路徑是根目錄或空路徑
        path = "/index.htm"; // 將根路徑存為 index.htm (網頁爬蟲的標準作法)
    }
    // 確保路徑以 / 開頭 (因為 output_dir 結尾沒有 /)，並加上 output_dir
    if (path[0] != '/') path = "/" + path;
    return output_dir + path; // 回傳完整的本地路徑 (例如 download/index.htm)
}

// ----------------------------------------------------------------------
// 輔助函式：計算相對路徑 (核心離線瀏覽技巧)
// ----------------------------------------------------------------------
// 【修正】：移除 static
std::string relative_path(const std::string& current_local_path_rel, // 當前 HTML 檔案的路徑 (例如 /about/page.html)
                                 const std::string& target_local_path_rel) { // 目標資源的路徑 (例如 /images/cat.jpg)
    // 函式用於將絕對路徑 (例如 /images/cat.jpg) 轉換成相對路徑 (例如 ../../images/cat.jpg)

    std::string path1 = current_local_path_rel.substr(1); // 移除開頭的 / (例如 about/page.html)
    std::string path2 = target_local_path_rel.substr(1);  // 移除開頭的 / (例如 images/cat.jpg)

    // 定義一個 Lambda 函式來將路徑字串依據 '/' 拆分成多個部分 (例如 "about/page.html" -> ["about", "page.html"])
    auto split_path = [](const std::string& p) {
        std::vector<std::string> parts;
        if (p.empty()) return parts;
        std::size_t start = 0;
        std::size_t end;
        // 尋找每一個 '/' 的位置
        while ((end = p.find('/', start)) != std::string::npos) {
            if (end > start) parts.push_back(p.substr(start, end - start)); // 取出 '/' 之間的字串
            start = end + 1; // 從 '/' 的下一位開始找
        }
        if (start < p.size()) parts.push_back(p.substr(start)); // 處理最後剩下的部分
        return parts;
    };
    
    std::string dir1 = path1;
    std::size_t slash = path1.rfind('/'); // 找到當前檔案名之前的最後一個 '/'
    if (slash != std::string::npos) dir1 = path1.substr(0, slash); // 取得當前檔案所在的目錄 (例如 about)
    else dir1 = ""; // 如果路徑中沒有 '/'，則在根目錄下

    std::vector<std::string> parts1 = split_path(dir1); // 拆分當前目錄的路徑部分
    std::vector<std::string> parts2 = split_path(path2); // 拆分目標資源的路徑部分 (包含檔名)

    int common = 0; // 計算兩個路徑共同的層數
    // 找出兩個路徑從頭開始共同的部分
    for (size_t i = 0; i < parts1.size() && i < parts2.size(); ++i) {
        if (parts1[i] == parts2[i]) {
            common++; // 如果路徑名相同，共同層數加一
        } else {
            break; // 遇到不同的部分就停止
        }
    }

    std::string result = "";
    // 回退層數：對於當前目錄路徑 (parts1) 中，從共同部分之後的每一層，都需要用 "../" 回退
    for (size_t i = common; i < parts1.size(); ++i) {
        result += "../";
    }

    // 加上目標路徑的剩餘部分 (從共同部分之後的部分)
    for (size_t i = common; i < parts2.size(); ++i) {
        result += parts2[i];
        if (i < parts2.size() - 1) {
            result += "/"; // 如果不是最後一個路徑部分 (檔名)，就加上 '/'
        }
    }
    
    return result; // 回傳最終的相對路徑
}


// ----------------------------------------------------------------------
// 連結擷取函式 (支援 HTML/CSS 連結)
// ----------------------------------------------------------------------
// 【修正】：移除 static
void extract_links(const std::string& content,          // 要解析的 HTML/CSS 內容
                          const Url& base_url,         // 當前頁面 URL (用於組成絕對路徑)
                          bool allow_external,         // 是否允許外部連結 (由 -e 參數決定)
                          std::vector<std::string>& out) { // 輸出：找到的合法連結列表
    
    // 定義一個 Lambda 函式來處理找到的連結字串，將其正規化並加入輸出列表
    auto add_link = [&](const std::string& link) {
        // 忽略空連結、錨點連結 (#section) 或查詢參數連結 (?param)
        if (link.empty() || link.front() == '#' || link.front() == '?') return; 
        
        // 將相對連結 (例如 /images/cat.jpg) 轉換為完整的絕對 URL
        std::string full = join_url(base_url, link);

        Url u;
        // 檢查是否為 http:// 開頭，並且能否成功解析
        if (full.find("http://") != 0 || !parse_url(full, u)) return; 

        // 外站限制檢查：如果 cfg.allow_external 為 false，且主機名稱不同
        if (!allow_external && u.host != base_url.host) {
            return; // 忽略外部連結
        }
        out.push_back(full); // 加入合法連結到輸出列表
    };

    std::size_t pos = 0; // 當前在 content 中開始尋找的位置
    while (true) {
        std::size_t p = std::string::npos; // 記錄找到的最近位置
        char quote = '\0'; // 記錄連結使用的引號類型 (單引號或雙引號)
        std::string link_tag = ""; // 記錄匹配到的標籤前綴

        // 查找各種常見的連結標籤開頭
        std::size_t h_d = content.find("href=\"", pos); // 查找 href="
        std::size_t h_s = content.find("href='", pos); // 查找 href='
        std::size_t s_d = content.find("src=\"", pos);  // 查找 src="
        std::size_t s_s = content.find("src='", pos);  // 查找 src='
        std::size_t u_low = content.find("url(", pos); // 查找 CSS 中的 url(

        // 使用 std::min 找到這五種標籤中，在當前位置 (pos) 之後出現的最早位置
        p = std::min({
            h_d == std::string::npos ? content.size() : h_d,
            h_s == std::string::npos ? content.size() : h_s,
            s_d == std::string::npos ? content.size() : s_d,
            s_s == std::string::npos ? content.size() : s_s,
            u_low == std::string::npos ? content.size() : u_low
        });
        
        if (p == content.size()) break; // 如果 p 還是等於 content.size()，表示找不到任何標籤，結束迴圈

        // 確定是哪種標籤並設定前綴和引號
        if (p == h_d) { link_tag = "href=\""; quote = '"'; }
        else if (p == h_s) { link_tag = "href='"; quote = '\''; }
        else if (p == s_d) { link_tag = "src=\""; quote = '"'; }
        else if (p == s_s) { link_tag = "src='"; quote = '\''; }
        else if (p == u_low) { link_tag = "url("; quote = '\0'; } 

        std::size_t start = p + link_tag.size(); // 連結內容的起始位置 (在引號或括號之後)
        std::size_t end; // 連結內容的結束位置
        
        // 專門處理 url(...) 格式
        if (link_tag == "url(") {
            char search_quote = '\0';
            // 檢查 url( 之後是否有引號 (例如 url("img.jpg"))
            if (start < content.size() && (content[start] == '\'' || content[start] == '"')) {
                search_quote = content[start]; // 記錄引號
                start++; // 跳過開頭的引號
            }
            if (search_quote != '\0') {
                end = content.find(search_quote, start); // 尋找結尾引號
            } else {
                end = content.find(')', start); // 如果沒引號，尋找 ')'
            }
            
            // 錯誤處理
            if (end == std::string::npos || end <= start) {
                pos = start;
                continue; 
            }
            
            // 取得連結內容
            std::string link = content.substr(start, end - start);
            // 清理 url() 尾部可能的多餘字元 (如空白、多餘的引號/括號)
            while (!link.empty() && (link.back() == ')' || link.back() == '\'' || link.back() == '"')) {
                link.pop_back();
            }
            add_link(link); // 加入連結
            pos = end + (search_quote != '\0' ? 1 : 0); // 更新下次查找位置 (跳過結尾引號或括號)
            
        } 
        // 處理 href/src 格式
        else {
            end = content.find(quote, start); // 尋找結尾的引號
            if (end == std::string::npos) {
                pos = start; 
                continue; 
            }
            std::string link = content.substr(start, end - start); // 取得連結內容
            add_link(link); // 加入連結
            pos = end + 1; // 更新下次查找位置 (跳過結尾引號)
        }
    }
}

// ----------------------------------------------------------------------
// 連結重寫函式 (核心離線功能)
// ----------------------------------------------------------------------
// 【修正】：移除 static
std::string rewrite_content(const std::string& content,         // 原始檔案內容
                                   const Url& base_url,        // 當前頁面的 URL (用於計算絕對 URL)
                                   const std::string& current_local_path, // 當前檔案的本地路徑 (例如 download/about/page.html)
                                   const std::string& output_dir) { // 輸出目錄 (例如 download)
    
    std::string rewritten_content = content; // 複製一份內容，用於修改
    // 定義要尋找的連結標籤前綴
    std::vector<std::string> tags = {"href=\"", "src=\"", "href='", "src='", "url("};

    for (const std::string& tag_prefix : tags) { // 遍歷每種標籤
        std::size_t tag_pos = 0; // 查找的起始位置
        
        while (true) {
            tag_pos = rewritten_content.find(tag_prefix, tag_pos); // 尋找下一個標籤
            if (tag_pos == std::string::npos) break; // 找不到就跳出
            
            bool is_quoted = (tag_prefix.back() == '"' || tag_prefix.back() == '\'');
            char original_quote = is_quoted ? tag_prefix.back() : '\0'; // 取得原始引號

            std::size_t start = tag_pos + tag_prefix.size(); // 連結內容的起始位置
            std::size_t end; // 連結內容的結束位置
            
            // 處理 url(...) 中可能出現的引號
            char search_quote = original_quote;
            if (tag_prefix == "url(") {
                // 檢查 url( 之後是否有引號
                if (start < rewritten_content.size() && (rewritten_content[start] == '\'' || rewritten_content[start] == '"')) {
                    search_quote = rewritten_content[start];
                    start++; // 跳過開頭的引號
                }
            }
            
            // 確定連結內容的結束位置
            if (search_quote != '\0') {
                end = rewritten_content.find(search_quote, start);
            } else {
                end = rewritten_content.find(')', start);
            }

            // 錯誤或找不到結尾
            if (end == std::string::npos || end <= start) {
                tag_pos = start;
                continue;
            }

            std::string link = rewritten_content.substr(start, end - start); // 取得連結字串
            // 清理 url() 尾部的多餘字元
            while (!link.empty() && (link.back() == ')' || link.back() == '\'' || link.back() == '"')) {
                link.pop_back();
            }

            // 1. 轉成絕對 URL
            std::string full_url_str = join_url(base_url, link);
            
            // 2. 解析 URL
            Url target_url;
            // 檢查：a) 是不是 http:// 開頭 b) 解析是否成功 c) 是不是外站
            if (full_url_str.find("http://") != 0 || !parse_url(full_url_str, target_url) || target_url.host != base_url.host) {
                tag_pos = end + 1; // 繼續從下一位開始找
                continue; 
            }

            // 3. 得到目標本機路徑
            std::string target_local_path = url_to_local_path(target_url, output_dir);
            
            // 4. 計算相對路徑：這一步是核心離線瀏覽的實現
            // current_path_rel = /about/page.html -> /about/page.html
            std::string current_path_rel = current_local_path.substr(output_dir.size());
            // target_path_rel = /images/cat.jpg -> /images/cat.jpg
            std::string target_path_rel = target_local_path.substr(output_dir.size());

            // 呼叫輔助函式，得到相對路徑 (例如 ../images/cat.jpg)
            std::string new_link = relative_path(current_path_rel, target_path_rel); 

            // 5. 替換
            std::size_t original_len = end - start; // 原始連結的長度
            std::size_t rewritten_offset = start; // 替換開始的位置
            
            // 將原始連結字串替換為新的相對路徑
            rewritten_content.replace(rewritten_offset, original_len, new_link);

            // 更新下次查找位置：由於字串長度可能改變，需要手動計算下一次尋找標籤的位置
            std::ptrdiff_t length_diff = new_link.size() - original_len;
            // tag_pos = 結尾位置 + 長度差 + (是否跳過結尾引號/括號)
            tag_pos = end + length_diff + (search_quote != '\0' ? 1 : 0);
        }
    }

    return rewritten_content; // 回傳重寫後的內容
}


// ----------------------------------------------------------------------
// 爬蟲主邏輯
// ----------------------------------------------------------------------
void crawl_site(const Config& cfg) {
    Url start_url;
    // 解析起始 URL
    if (!parse_url(cfg.url_str, start_url)) {
        // 此處不需處理 HTTPS，因為老師要求只測試 HTTP
        std::cerr << "URL 解析失敗: " << cfg.url_str << "\n";
        return;
    }

    if (!cfg.resume) {
        // -c 0: 重新下載模式，先刪除舊的輸出目錄 (破壞性操作)
        std::string cmd = "rm -rf '" + cfg.output_dir + "'"; // 使用 rm -rf 刪除目錄及其內容
        std::system(cmd.c_str()); // 執行系統命令
    }
    ensure_dir(cfg.output_dir); // 確保輸出根目錄存在 (如果被刪除或從未建立)

    // 定義 Node 結構，用於儲存佇列中的工作項目
    struct Node {
        std::string url_str; // 連結字串
        int depth;           // 該連結所在的深度
    };

    std::queue<Node> q;        // BFS 工作佇列
    std::set<std::string> visited; // 追蹤已訪問的 URL (防止重複)

    // 啟動：只加入起始 URL
    q.push({cfg.url_str, 0}); // 深度為 0
    visited.insert(cfg.url_str); // 標記為已訪問

    DownloadStats global_stats; // 全局統計資訊結構
    // 記錄開始時間，用於計算總下載時間和即時速度
    global_stats.start_time = std::chrono::steady_clock::now();

    while (!q.empty()) { // 只要佇列中還有待處理的網址
        Node cur = q.front();
        q.pop(); // 取出並移除佇列最前端的網址

        Url u;
        if (!parse_url(cur.url_str, u)) continue; // 再次解析 URL (如果失敗則跳過)

        std::string local_path = url_to_local_path(u, cfg.output_dir); // 決定檔案儲存的本地路徑

        // 判斷檔案類型 (用於決定深度是否增加，以及是否進行連結解析/重寫)
        bool is_html = (local_path.size() >= 4 &&
                        (local_path.substr(local_path.size() - 4) == ".htm" || // 檢查 .htm 結尾
                         local_path.substr(local_path.size() - 5) == ".html"));// 檢查 .html 結尾
        bool is_css = (local_path.size() >= 4 && local_path.substr(local_path.size() - 4) == ".css"); // 檢查 .css 結尾
        
        bool should_download = true; // 預設應下載

        if (cfg.resume && file_exists(local_path)) { // -c 1 續傳模式檢查
            // -c 1: 續傳模式，如果檔案已存在
            std::cout << "[跳過已存在] " << local_path << "\n";
            should_download = false;
        } 
        
        if (should_download) {
            std::cout << "\n[取回] depth=" << cur.depth << " " << cur.url_str
                      << " -> " << local_path << "\n";

            // 呼叫下載模組 (http_download_to_file 會在下載失敗時回傳 false)
            bool ok = http_download_to_file(u, local_path, &global_stats);
            if (!ok) {
                std::cerr << "[下載失敗] " << cur.url_str << "\n";
                continue; // 下載失敗，跳過後續的解析/重寫步驟
            }
        }
        
        // 深度限制檢查：如果當前深度已達上限，且是 HTML 頁面，則不進行連結解析
        if (cur.depth >= cfg.depth && is_html) continue;

        // 僅對 HTML 和 CSS 檔案進行連結解析/重寫 (因為其他檔案類型不包含連結)
        if (!is_html && !is_css) continue; 


        // 讀檔，準備解析/重寫 (使用 std::ios::binary 確保讀取二進制資料完整)
        std::ifstream fin(local_path.c_str(), std::ios::binary); 
        if (!fin) {
            std::cerr << "[錯誤] 無法讀取檔案以進行解析/重寫: " << local_path << "\n";
            continue;
        }
        // 使用高效的迭代器將整個檔案內容讀取到 std::string 中
        std::string content((std::istreambuf_iterator<char>(fin)),
                            std::istreambuf_iterator<char>());
        fin.close();

        // 核心：連結重寫 (實作離線瀏覽要求)
        std::string processed_content = rewrite_content(content, u, local_path, cfg.output_dir); 

        // 重新寫回檔案 (使用 std::ios::binary 確保寫入二進制資料完整)
        std::ofstream fout(local_path.c_str(), std::ios::trunc | std::ios::binary); // ios::trunc 確保檔案被清空
        if (fout) {
            fout.write(processed_content.data(), processed_content.size()); // 寫入重寫後的內容
            fout.close();
        } else {
            std::cerr << "[警告] 無法重新寫入檔案以重寫連結: " << local_path << "\n";
        }

        // 解析連結：從重寫後的內容中提取新的 URL
        std::vector<std::string> links;
        extract_links(processed_content, u, cfg.allow_external, links); 

        for (const std::string& link_url : links) { // 遍歷所有新提取的連結
            // visited.insert(link_url).second: 嘗試將連結加入集合，如果成功 (回傳 true)，表示這是新連結
            if (visited.insert(link_url).second) {
                // 決定下一個連結的深度：如果是 HTML 連結，深度 +1；如果是 CSS 等資源連結，深度不變
                int next_depth = is_html ? cur.depth + 1 : cur.depth;
                if (next_depth <= cfg.depth) {
                    q.push({link_url, next_depth}); // 將新連結加入佇列等待處理
                }
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    // 計算總執行時間 (秒)
    double elapsed = std::chrono::duration<double>(end - global_stats.start_time).count();

    std::cout << "\n=== 下載統計資訊 ===\n";
    std::cout << " 已下載檔案數量 : " << global_stats.total_files << "\n"; // 輸出總檔案數
    std::cout << " 已下載總容量   : " << global_stats.total_bytes << " bytes\n"; // 輸出總位元組數
    std::cout << " 已下載時間     : " << elapsed << " 秒\n";
    std::cout << "====================\n";
}
