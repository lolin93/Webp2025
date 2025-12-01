// http_client.cpp 最上面保留這些 include
#include "http_client.h"  // 包含函式宣告、Url 結構和 DownloadStats
#include <sys/socket.h>   // 包含 socket 相關的系統呼叫
#include <netdb.h>        // 包含 getaddrinfo/addrinfo 相關的函式
#include <unistd.h>       // 包含 close() 函式 (關閉 socket)
#include <cstring>        // 包含 memset/strcmp 等記憶體和字串操作
#include <cstdio>         // 包含 C 語言檔案操作 (fopen, fclose, fwrite, snprintf)
#include <iostream>       // 包含標準輸出入 (std::cout, std::cerr)
#include <string>         // 包含 std::string
#include <sys/stat.h>     // 包含 stat 結構 (雖然這裡用不到，但為了相容性保留)
#include <cstdlib>        // 包含 system() (用於 ensure_parent_dir)
#include <algorithm>      // 包含標準演算法
#include <chrono>         // 包含時間相關函式 (計算速度和 ETA)


// 建立上層目錄，例如 filename = "download/images/cat1.jpg"
// 會呼叫：mkdir -p "download/images"
// 這是為了確保檔案可以被正確寫入到深層目錄中。
bool ensure_parent_dir(const std::string& filename) {
    // 找出最後一個 '/' 的位置，它分隔了目錄和檔案名
    std::size_t slash = filename.rfind('/');
    // 如果找不到 '/'，表示路徑中沒有子資料夾，例如 filename="index.htm"。
    // 這種情況下，父目錄就是當前目錄，所以直接回傳 true。
    if (slash == std::string::npos) return true;

    // 擷取從開頭到最後一個 '/' 之前的部分，即為父目錄路徑
    std::string dir = filename.substr(0, slash);  // 例如 "download/images"
    // 如果目錄字串為空（理論上不該發生），也回傳 true。
    if (dir.empty()) return true;

    // 組合系統命令：使用 mkdir -p 遞迴創建多層目錄。
    // 使用單引號 (') 包住路徑，以避免路徑中包含空格或特殊字符時出錯。
    std::string cmd = "mkdir -p '" + dir + "'";
    
    // 執行系統命令
    int ret = std::system(cmd.c_str());
    
    // 檢查系統命令的執行結果。0 表示成功。
    return (ret == 0);
}


// 把 bytes 轉成 KB/MB 字串，用於美化輸出
std::string fmt_size(std::size_t bytes) {
    char buf[64];
    if (bytes < 1024) {
        // 小於 1KB 顯示為 B (Bytes)
        std::snprintf(buf, sizeof(buf), "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        // 小於 1MB 顯示為 KB (Kilobytes)，保留一位小數
        std::snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
    } else {
        // 大於等於 1MB 顯示為 MB (Megabytes)，保留兩位小數
        std::snprintf(buf, sizeof(buf), "%.2f MB", bytes / 1024.0 / 1024.0);
    }
    return std::string(buf);
}

// 核心下載函式：從 URL 下載內容並存到檔案
bool http_download_to_file(const Url& url,
                           const std::string& output_path,
                           DownloadStats* global_stats) {
    
    // ------------------------------------------------------------------
    // 1. 解析 Hostname → IP Address (使用 getaddrinfo)
    // ------------------------------------------------------------------
    struct addrinfo hints;
    // 將 hints 結構體清零
    std::memset(&hints, 0, sizeof(hints));
    // 設置為 IPv4 協定 (AF_INET)
    hints.ai_family     = AF_INET;
    // 設置為 TCP 流式 Socket
    hints.ai_socktype   = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    // 將 Port 號碼轉換為字串，供 getaddrinfo 使用
    std::string port_str = std::to_string(url.port);

    // 呼叫 getaddrinfo 進行 DNS 解析和協定配置
    int ret = getaddrinfo(url.host.c_str(), port_str.c_str(), &hints, &res);
    // 檢查 getaddrinfo 是否成功
    if (ret != 0 || !res) {
        std::cerr << "getaddrinfo 失敗: " << gai_strerror(ret) << "\n";
        return false;
    }
    // 

    // ------------------------------------------------------------------
    // 2. 建立 Socket 連線
    // ------------------------------------------------------------------
    // 根據 getaddrinfo 的結果建立 socket
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        std::perror("socket");
        freeaddrinfo(res); // 失敗時釋放 getaddrinfo 資源
        return false;
    }
    // 連接到遠端伺服器
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        std::perror("connect");
        close(sock);
        freeaddrinfo(res);
        return false;
    }
    // 釋放 getaddrinfo 分配的鏈表資源
    freeaddrinfo(res);

    // ------------------------------------------------------------------
    // 3. 組合 HTTP GET 請求並發送
    // ------------------------------------------------------------------
    // 確保路徑非空，如果為空則設為根路徑 "/"
    std::string path = url.path.empty() ? "/" : url.path;
    // 組合 HTTP/1.1 GET 請求頭
    std::string req =
        "GET " + path + " HTTP/1.1\r\n"      // 請求行: 方法, 路徑, 協定版本
        "Host: " + url.host + "\r\n"         // Host 標頭: 必須提供
        "Connection: close\r\n"              // Connection 標頭: 請求伺服器完成後關閉連線
        "\r\n";                              // 空行: 標記請求頭結束，後接請求體 (GET 無請求體)

    // 發送請求
    const char* p = req.c_str();
    size_t left = req.size();
    // 循環發送，確保整個請求都被送出 (因為 send 不保證一次發送所有資料)
    while (left > 0) {
        ssize_t n = send(sock, p, left, 0);
        if (n <= 0) {
            std::perror("send");
            close(sock);
            return false;
        }
        p      += n;
        left   -= n;
    }

    // ------------------------------------------------------------------
    // 4. 建立輸出檔案
    // ------------------------------------------------------------------
    // 確保檔案的父目錄存在 (例如 /download/host/images/)
    ensure_parent_dir(output_path); 
    
    // 使用 C 語言的 FILE 結構開啟檔案，使用 "wb" 模式 (寫入，二進制模式)
    FILE* fp = std::fopen(output_path.c_str(), "wb"); 
    if (!fp) {
        std::perror("fopen");
        close(sock);
        return false;
    }

    // ------------------------------------------------------------------
    // 5. 讀取 HTTP 回應 (Header + Body)
    // ------------------------------------------------------------------
    const size_t BUF_SIZE = 8192; // 定義緩衝區大小
    char buf[BUF_SIZE];
    bool header_done = false;     // 標記是否已讀完回應頭
    std::string header_buf;       // 暫存回應頭的緩衝區

    std::size_t content_length = 0;
    bool has_content_length = false;

    std::size_t downloaded = 0; // 已下載的內容長度 (Bytes)
    // 記錄下載開始時間 (用於計算速度)
    auto start = std::chrono::steady_clock::now();
    // 如果是第一次下載，更新全域統計的開始時間
    if (global_stats && global_stats->start_time.time_since_epoch().count() == 0) {
        global_stats->start_time = start;
    }

    // 主接收迴圈：持續從 socket 接收資料，直到連線關閉
    while (true) {
        // 接收資料到緩衝區
        ssize_t n = recv(sock, buf, BUF_SIZE, 0);
        
        if (n < 0) {
            std::perror("recv");
            std::fclose(fp);
            close(sock);
            return false;
        }
        // 如果 n == 0，表示伺服器關閉連線 (下載完成)
        if (n == 0) break;

        // 處理回應頭 (Header)
        if (!header_done) {
            // 將接收到的資料添加到回應頭緩衝區
            header_buf.append(buf, n);
            // 檢查是否找到回應頭結束標記 "\r\n\r\n"
            std::size_t pos = header_buf.find("\r\n\r\n");
            
            if (pos != std::string::npos) {
                // --- 解析 Content-Length ---
                std::string header_text = header_buf.substr(0, pos);
                std::size_t cl_pos = header_text.find("Content-Length:");
                if (cl_pos != std::string::npos) {
                    // 提取 Content-Length 的數值
                    cl_pos += 15; // 跳過 "Content-Length:"
                    while (cl_pos < header_text.size() && (header_text[cl_pos] == ' ' || header_text[cl_pos] == '\t'))
                        ++cl_pos; // 跳過空白
                    std::size_t end = cl_pos;
                    while (end < header_text.size() && header_text[end] >= '0' && header_text[end] <= '9')
                        ++end; // 找到數字結尾
                    std::string num = header_text.substr(cl_pos, end - cl_pos);
                    if (!num.empty()) {
                        content_length = static_cast<std::size_t>(std::stoll(num));
                        has_content_length = true;
                    }
                }

                // --- 寫入 Body 部分 ---
                header_done = true;
                std::size_t body_start = pos + 4; // 找到 Body 開始的位置 (+4 是跳過 "\r\n\r\n")
                std::size_t body_len   = header_buf.size() - body_start; // 計算已接收到的 Body 長度
                if (body_len > 0) {
                    // 將 Body 部分寫入檔案
                    std::fwrite(header_buf.data() + body_start, 1, body_len, fp);
                    downloaded += body_len;
                }
                // 清空回應頭緩衝區，釋放記憶體
                header_buf.clear();
                header_buf.shrink_to_fit();
            }
        } else {
            // 處理回應體 (Body)：回應頭已讀完，直接將所有接收到的資料寫入檔案
            std::fwrite(buf, 1, n, fp);
            downloaded += n;
        }

        // ------------------------------------------------------------------
        // 6. 顯示進度 (計算速度和 ETA)
        // ------------------------------------------------------------------
        auto now = std::chrono::steady_clock::now();
        // 計算總共經過的時間 (秒)
        double elapsed = std::chrono::duration<double>(now - start).count();
        // 計算下載速度 (Bytes/秒)
        double speed = elapsed > 0 ? downloaded / elapsed : 0.0; 

        double percent = 0.0;
        // 如果有 Content-Length，計算下載百分比
        if (has_content_length && content_length > 0) {
            percent = downloaded * 100.0 / content_length;
        }

        double eta = 0.0;
        // 如果有 Content-Length 且有速度，計算預估剩餘時間 (ETA)
        if (has_content_length && speed > 0 && content_length > downloaded) {
            eta = (content_length - downloaded) / speed;
        }

        // 輸出進度條：使用 \r (Carriage Return) 讓游標回到行首，覆蓋前一次輸出
        std::cout << "\r[下載中] "
                  << fmt_size(downloaded); // 顯示已下載大小
        if (has_content_length) {
            std::cout << " / " << fmt_size(content_length) // 顯示總大小
                      << " (" << (int)percent << "%)";    // 顯示百分比
        }
        if (speed > 0) {
            std::cout << "  速度: " << fmt_size((std::size_t)speed) << "/s";
        }
        if (eta > 0) {
            std::cout << "  預估剩餘: " << (int)eta << " 秒";
        }
        // 額外空格用於清除行尾殘留文字，並使用 std::flush 確保立即輸出
        std::cout << "      " << std::flush;
    }

    // ------------------------------------------------------------------
    // 7. 結束清理
    // ------------------------------------------------------------------
    std::fclose(fp); // 關閉檔案
    close(sock);     // 關閉 socket 連線
    
    // 輸出最終完成訊息
    std::cout << "\r[完成]  " << output_path << "  (" << fmt_size(downloaded) << ")\n";

    // 更新全域統計資訊
    if (global_stats) {
        global_stats->total_files += 1;
        global_stats->total_bytes += downloaded;
    }

    return true; // 下載成功
}
