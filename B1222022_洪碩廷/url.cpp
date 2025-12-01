#include "url.h"
#include <cstddef>
#include <string>
#include <stdexcept> // 為了 std::stoi

// ----------------------------------------------------------------------
// 函式：parse_url (解析完整 URL 字串)
// ----------------------------------------------------------------------
// 將完整的 URL 字串 (s) 拆解成結構化的 Url 物件 (out)。
bool parse_url(const std::string& s, Url& out) {
    // 初始化 Url 結構體，確保所有成員都是空值或預設值。
    out = Url();

    // 複製輸入的 URL 字串到一個可修改的變數。
    std::string url = s;
    
    // 簡單處理前後空白：移除 URL 字串尾部的空白字元或換行符號。
    while (!url.empty() && (url.back() == ' ' || url.back() == '\n' || url.back() == '\r' || url.back() == '\t'))
        url.pop_back();

    // 定義我們支援的協議前綴，目前只處理 http。
    const std::string prefix = "http://";
    
    // 檢查 URL 是否以 "http://" 開頭。
    if (url.compare(0, prefix.size(), prefix) != 0) {
        return false; // 如果不是，解析失敗，回傳 false。
    }
    // 設定協議為 "http"。
    out.scheme = "http";

    // 紀錄協議後的位置 (即從 "http://" 後開始尋找主機名和路徑)。
    std::size_t pos = prefix.size();
    
    // 尋找主機名後面的第一個斜線 '/'，這是主機/埠號和路徑的分界線。
    std::size_t slash_pos = url.find('/', pos);
    
    // 用來暫存主機名和埠號的部分 (例如 "hsccl.fr.to" 或 "localhost:8080")。
    std::string host_port;
    
    // 情況 A: 如果找不到斜線 (例如輸入 "http://google.com")。
    if (slash_pos == std::string::npos) {
        // host_port 就是從 pos 開始到結尾的全部。
        host_port = url.substr(pos);
        // 路徑預設為根路徑。
        out.path = "/";
    } else {
        // 情況 B: 找到斜線 (例如輸入 "http://google.com/search")。
        // host_port 是從 pos 開始到 slash_pos 之前的部分。
        host_port = url.substr(pos, slash_pos - pos);
        // path 是從 slash_pos 開始到結尾的部分。
        out.path = url.substr(slash_pos);
    }

    // 尋找 host_port 中是否有冒號 ':'，這是主機名和埠號的分界線。
    std::size_t colon_pos = host_port.find(':');
    
    // 情況 A: 如果沒有冒號 (例如 "hsccl.fr.to")。
    if (colon_pos == std::string::npos) {
        // 整個 host_port 就是主機名。
        out.host = host_port;
        // 埠號使用 HTTP 預設值 80。
        out.port = 80;
    } else {
        // 情況 B: 找到冒號 (例如 "localhost:8080")。
        // 主機名是冒號之前的部分。
        out.host = host_port.substr(0, colon_pos);
        // 埠號字串是冒號之後的部分。
        std::string port_str = host_port.substr(colon_pos + 1);
        try {
            // 嘗試將埠號字串轉換為整數。
            out.port = std::stoi(port_str);
        } catch (...) {
            // 如果轉換失敗 (例如埠號是亂碼)，忽略錯誤。
            // 使用預設埠號 80 作為後備值。
            out.port = 80; 
        }
    }

    // 最後檢查：確保路徑絕對不能為空，如果為空則設定為根路徑 "/"。
    if (out.path.empty())
        out.path = "/";

    // 解析成功，回傳 true。
    return true;
}

// ----------------------------------------------------------------------
// 函式：join_url (組合 URL)
// ----------------------------------------------------------------------
// 把一個基礎 URL (base) 和一個連結 (link) 組合，生成一個完整的絕對 URL。
std::string join_url(const Url& base, const std::string& link) {
    // 檢查埠號是否是標準的 80。如果是，則 port_str 為空；如果不是，則加上 ":[port]"。
    std::string port_str = (base.port == 80) ? "" : ":" + std::to_string(base.port);

    // 檢查 link 是否已經是一個完整的絕對網址 (以 http:// 或 https:// 開頭)。
    if (link.find("http://") == 0 || link.find("https://") == 0) {
        return link; // 如果是，直接回傳它。
    }
    
    // 檢查 link 是否是**絕對路徑** (以 '/' 開頭，例如 "/images/logo.png")。
    if (!link.empty() && link[0] == '/') {
        // 絕對路徑：從 base 的 host 根目錄開始組合。
        // 格式：[scheme]://[host][port]/[link]
        return base.scheme + "://" + base.host + port_str + link;
    }

    // 情況：**相對路徑** (例如 "cat.jpg" 或 "../images/dog.png")。
    // 相對路徑需要以 base.path 的目錄部分作為基準。
    std::string dir = base.path;
    
    // 找到 base.path 中最後一個 '/' 的位置。
    std::size_t slash = dir.rfind('/');
    
    // 情況 A: 如果路徑中沒有斜線 (這在實際的 URL 解析中很少見，但以防萬一)。
    if (slash == std::string::npos) {
        dir = "/"; // 預設為根目錄。
    } else {
        // 情況 B: 找到斜線。
        // 檢查斜線是否在結尾 (例如 /about/，slash < dir.size() - 1 為 false)。
        // 如果斜線不在結尾 (即路徑是一個檔案，如 /about/page.html)，則去掉檔名部分。
        if (slash < dir.size() - 1) {
            dir = dir.substr(0, slash + 1); // 截取到 '/' 為止，確保目錄以 '/' 結尾。
        }
        // 如果路徑已經是目錄 (例如 /about/)，則 slash == dir.size() - 1，dir 不變。
    }
    
    // 組合最終的 URL。
    // 格式：[scheme]://[host][port][base 的目錄部分][link 的相對路徑]
    return base.scheme + "://" + base.host + port_str + dir + link;
}
