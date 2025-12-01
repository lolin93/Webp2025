// http_client.cpp 最上面保留這些 include
#include "http_client.h"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <cstdlib>
#include <algorithm>
#include <chrono>


// 建立上層目錄，例如 filename = "download/images/cat1.jpg"
// 會呼叫：mkdir -p "download/images"
// 【修正】：移除 static
bool ensure_parent_dir(const std::string& filename) {
    std::size_t slash = filename.rfind('/');
    if (slash == std::string::npos) return true;   // 沒有子資料夾

    std::string dir = filename.substr(0, slash);   // 例如 "download/images"
    if (dir.empty()) return true;

    std::string cmd = "mkdir -p '" + dir + "'";
    int ret = std::system(cmd.c_str());
    return (ret == 0);
}


// 把 bytes 轉成 KB/MB 字串
// 【修正】：移除 static，並修正輸出格式以匹配 `crawler.cpp` 的期望
std::string fmt_size(std::size_t bytes) {
    char buf[64];
    if (bytes < 1024) {
        // 修正：將 "%zub" 改為 "%zu B" (更清晰)
        std::snprintf(buf, sizeof(buf), "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        std::snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
    } else {
        std::snprintf(buf, sizeof(buf), "%.2f MB", bytes / 1024.0 / 1024.0);
    }
    return std::string(buf);
}

bool http_download_to_file(const Url& url,
                           const std::string& output_path,
                           DownloadStats* global_stats) {
    // 1. 解析 host → IP
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    std::string port_str = std::to_string(url.port);

    int ret = getaddrinfo(url.host.c_str(), port_str.c_str(), &hints, &res);
    if (ret != 0 || !res) {
        std::cerr << "getaddrinfo 失敗: " << gai_strerror(ret) << "\n";
        return false;
    }

    // 2. 建立 socket & connect
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        std::perror("socket");
        freeaddrinfo(res);
        return false;
    }
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        std::perror("connect");
        close(sock);
        freeaddrinfo(res);
        return false;
    }
    freeaddrinfo(res);

    // 3. 組 GET
    std::string path = url.path.empty() ? "/" : url.path;
    std::string req =
        "GET " + path + " HTTP/1.1\r\n"
        "Host: " + url.host + "\r\n"
        "Connection: close\r\n"
        "\r\n";

    const char* p = req.c_str();
    size_t left = req.size();
    while (left > 0) {
        ssize_t n = send(sock, p, left, 0);
        if (n <= 0) {
            std::perror("send");
            close(sock);
            return false;
        }
        p    += n;
        left -= n;
    }

    // 4. 建立輸出檔案
    ensure_parent_dir(output_path); // 使用修正後的函式
    // ** 關鍵：使用 "wb" 二進制寫入模式 **
    FILE* fp = std::fopen(output_path.c_str(), "wb"); 
    if (!fp) {
        std::perror("fopen");
        close(sock);
        return false;
    }

    // 5. 讀 header + body
    const size_t BUF_SIZE = 8192;
    char buf[BUF_SIZE];
    bool header_done = false;
    std::string header_buf;

    std::size_t content_length = 0;
    bool has_content_length = false;

    std::size_t downloaded = 0;
    auto start = std::chrono::steady_clock::now();
    if (global_stats && global_stats->start_time.time_since_epoch().count() == 0) {
        global_stats->start_time = start;
    }

    while (true) {
        ssize_t n = recv(sock, buf, BUF_SIZE, 0);
        if (n < 0) {
            std::perror("recv");
            std::fclose(fp);
            close(sock);
            return false;
        }
        if (n == 0) break;

        if (!header_done) {
            header_buf.append(buf, n);
            std::size_t pos = header_buf.find("\r\n\r\n");
            if (pos != std::string::npos) {
                // 解析 Content-Length
                std::string header_text = header_buf.substr(0, pos);
                std::size_t cl_pos = header_text.find("Content-Length:");
                if (cl_pos != std::string::npos) {
                    cl_pos += 15;
                    while (cl_pos < header_text.size() && (header_text[cl_pos] == ' ' || header_text[cl_pos] == '\t'))
                        ++cl_pos;
                    std::size_t end = cl_pos;
                    while (end < header_text.size() && header_text[end] >= '0' && header_text[end] <= '9')
                        ++end;
                    std::string num = header_text.substr(cl_pos, end - cl_pos);
                    if (!num.empty()) {
                        content_length = static_cast<std::size_t>(std::stoll(num));
                        has_content_length = true;
                    }
                }

                header_done = true;
                std::size_t body_start = pos + 4;
                std::size_t body_len   = header_buf.size() - body_start;
                if (body_len > 0) {
                    std::fwrite(header_buf.data() + body_start, 1, body_len, fp);
                    downloaded += body_len;
                }
                header_buf.clear();
                header_buf.shrink_to_fit();
            }
        } else {
            std::fwrite(buf, 1, n, fp);
            downloaded += n;
        }

        // 顯示進度
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();
        double speed = elapsed > 0 ? downloaded / elapsed : 0.0; // bytes/sec

        double percent = 0.0;
        if (has_content_length && content_length > 0) {
            percent = downloaded * 100.0 / content_length;
        }

        double eta = 0.0;
        if (has_content_length && speed > 0 && content_length > downloaded) {
            eta = (content_length - downloaded) / speed;
        }

        std::cout << "\r[下載中] "
                  << fmt_size(downloaded); // 使用修正後的函式
        if (has_content_length) {
            std::cout << " / " << fmt_size(content_length) // 使用修正後的函式
                      << " (" << (int)percent << "%)";
        }
        if (speed > 0) {
            std::cout << "  速度: " << fmt_size((std::size_t)speed) << "/s"; // 使用修正後的函式
        }
        if (eta > 0) {
            std::cout << "  預估剩餘: " << (int)eta << " 秒";
        }
        std::cout << "      " << std::flush;
    }

    std::fclose(fp);
    close(sock);
    std::cout << "\r[完成]  " << output_path << "  (" << fmt_size(downloaded) << ")\n"; // 使用修正後的函式

    if (global_stats) {
        global_stats->total_files += 1;
        global_stats->total_bytes += downloaded;
    }

    return true;
}