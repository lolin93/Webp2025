// url.cpp
#include "url.h"
#include <cstddef>
#include <string>
#include <stdexcept> // 為了 std::stoi

bool parse_url(const std::string& s, Url& out) {
    out = Url();

    std::string url = s;
    // 簡單處理前後空白
    while (!url.empty() && (url.back() == ' ' || url.back() == '\n' || url.back() == '\r' || url.back() == '\t'))
        url.pop_back();

    const std::string prefix = "http://";
    if (url.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    out.scheme = "http";

    std::size_t pos = prefix.size();
    std::size_t slash_pos = url.find('/', pos);
    std::string host_port;
    if (slash_pos == std::string::npos) {
        host_port = url.substr(pos);
        out.path = "/";
    } else {
        host_port = url.substr(pos, slash_pos - pos);
        out.path = url.substr(slash_pos);
    }

    std::size_t colon_pos = host_port.find(':');
    if (colon_pos == std::string::npos) {
        out.host = host_port;
        out.port = 80;
    } else {
        out.host = host_port.substr(0, colon_pos);
        std::string port_str = host_port.substr(colon_pos + 1);
        try {
            out.port = std::stoi(port_str);
        } catch (...) {
            // 忽略轉換錯誤，使用預設 port
            out.port = 80; 
        }
    }

    if (out.path.empty())
        out.path = "/";

    return true;
}

// 把 base 與相對路徑組成新的 URL（給 crawler 用）
std::string join_url(const Url& base, const std::string& link) {
    // 確保 port 能夠被正確地加入 URL
    std::string port_str = (base.port == 80) ? "" : ":" + std::to_string(base.port);

    if (link.find("http://") == 0 || link.find("https://") == 0) {
        return link; // 已經是絕對網址
    }
    if (!link.empty() && link[0] == '/') {
        // 絕對路徑
        return base.scheme + "://" + base.host + port_str + link;
    }

    // 相對路徑：取 base.path 的目錄部分
    std::string dir = base.path;
    std::size_t slash = dir.rfind('/');
    if (slash == std::string::npos) {
        dir = "/";
    } else {
        // 如果不是以 / 結尾 (即路徑是一個檔案)，則去掉檔名部分
        if (slash < dir.size() - 1) {
            dir = dir.substr(0, slash + 1); // 包含 '/'
        }
    }
    
    // 確保相對路徑 join 時也包含 port
    return base.scheme + "://" + base.host + port_str + dir + link;
}