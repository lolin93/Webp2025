// main.cpp
// 整合：參數解析 + 呼叫 crawler

#include <iostream>
#include <string>
#include "config.h"
#include "url.h"
#include "crawler.h"



int main(int argc, char* argv[]) {
    Config cfg;

    bool has_args = parse_args(argc, argv, cfg); // 有沒有從命令列拿到 url

    if (!has_args || cfg.url_str.empty()) {
        // 互動模式
        interactive_input(cfg);
    }

    if (cfg.output_dir.empty()) {
        cfg.output_dir = "download";
    }

    print_config(cfg);

    std::cout << "起始 URL: " << cfg.url_str << "\n";

    crawl_site(cfg);

    std::cout << "全部下載流程結束。\n";
    return 0;
}


