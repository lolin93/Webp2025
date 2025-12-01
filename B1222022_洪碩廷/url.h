// url.h
#ifndef URL_H
#define URL_H

#include <string>

struct Url {
    std::string scheme; // "http"
    std::string host;
    int         port = 80;
    std::string path;   // "/index.htm" 等
};

bool parse_url(const std::string& s, Url& out);

// 把 base 與相對路徑組成新的 URL（給 crawler 用）
std::string join_url(const Url& base, const std::string& link);

#endif // URL_H