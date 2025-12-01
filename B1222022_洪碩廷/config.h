// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <iostream>
#include <stdexcept>

struct Config {
    std::string url_str;      // 起始 URL
    std::string output_dir;   // 輸出目錄
    int depth = 0;            // 遞迴深度
    bool allow_external = false; // 是否允許外站
    bool resume = false;      // true = 繼續下載, false = 重新下載(砍資料夾)
    
    // 【新增功能】
    int num_threads = 1;               // -N: 同時下載執行緒數量 (預設 1)
    std::string allowed_extensions;    // -T: 允許的副檔名列表 (逗號分隔)
    std::size_t max_file_size_kb = 0;  // -S: 檔案大小上限 (KB, 0 = 無限制)
};

// 命令列參數解析：
inline bool parse_args(int argc, char* argv[], Config& cfg) {
    if (argc < 2) {
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        try {
            if (arg == "-u" && i + 1 < argc) {
                cfg.url_str = argv[++i];
            } else if ((arg == "-O" || arg == "-o") && i + 1 < argc) {
                cfg.output_dir = argv[++i];
            } else if (arg == "-d" && i + 1 < argc) {
                cfg.depth = std::stoi(argv[++i]);
            } else if (arg == "-e" && i + 1 < argc) {
                int v = std::stoi(argv[++i]);
                cfg.allow_external = (v != 0);
            } else if (arg == "-c" && i + 1 < argc) {
                int v = std::stoi(argv[++i]);
                cfg.resume = (v != 0);
            } 
            // 【新增參數解析】
            else if (arg == "-N" && i + 1 < argc) {
                cfg.num_threads = std::stoi(argv[++i]);
                if (cfg.num_threads < 1) cfg.num_threads = 1;
            } else if (arg == "-T" && i + 1 < argc) {
                cfg.allowed_extensions = argv[++i];
            } else if (arg == "-S" && i + 1 < argc) {
                cfg.max_file_size_kb = std::stoul(argv[++i]);
            }
            // ------------------------
            else {
                std::cerr << "未知參數: " << arg << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "參數錯誤: " << arg << " 需要數字.\n";
            return false;
        }
    }

    return !cfg.url_str.empty();
}

// 互動模式輸入 (已新增 -N, -T, -S 輸入)
inline void interactive_input(Config& cfg) {
    std::cout << "=== Downloader 互動模式 ===\n";

    std::cout << "請輸入 URL: ";
    std::getline(std::cin, cfg.url_str);

    std::cout << "請輸入輸出目錄 (例如 download): ";
    std::string tmp;
    std::getline(std::cin, tmp);
    cfg.output_dir = tmp.empty() ? "download" : tmp;

    std::cout << "請輸入遞迴深度 depth (0 = 不遞迴): ";
    std::getline(std::cin, tmp);
    cfg.depth = tmp.empty() ? 0 : std::stoi(tmp);

    std::cout << "是否允許外站？(0 = 否, 1 = 是): ";
    std::getline(std::cin, tmp);
    cfg.allow_external = (!tmp.empty() && tmp != "0");

    std::cout << "續傳模式？(0 = 重來並刪除舊目錄, 1 = 繼續下載): ";
    std::getline(std::cin, tmp);
    cfg.resume = (!tmp.empty() && tmp != "0");
    
    // 【新增互動輸入】
    std::cout << "同時下載執行緒數 (-N, 例如 4): ";
    std::getline(std::cin, tmp);
    cfg.num_threads = tmp.empty() ? 1 : std::stoi(tmp);
    if (cfg.num_threads < 1) cfg.num_threads = 1;

    std::cout << "允許的副檔名 (-T, 例如 html,jpg): ";
    std::getline(std::cin, cfg.allowed_extensions);

    std::cout << "檔案大小限制 (KB, -S, 0 = 無限制): ";
    std::getline(std::cin, tmp);
    cfg.max_file_size_kb = tmp.empty() ? 0 : std::stoul(tmp);
}

// 輸出設定 (已新增 -N, -T, -S 顯示)
inline void print_config(const Config& cfg) {
    std::cout << "[命令列模式]  下載設定 ==========\n";
    std::cout << " URL             : " << cfg.url_str << "\n";
    std::cout << " 輸出目錄        : " << cfg.output_dir << "\n";
    std::cout << " 深度 (depth)    : " << cfg.depth << "\n";
    std::cout << " 允許外站        : " << (cfg.allow_external ? "是" : "否") << "\n";
    std::cout << " 續傳模式(-c)    : " << (cfg.resume ? "繼續下載" : "重新下載") << "\n";
    
    // 【新增顯示】
    std::cout << " 同時執行緒數(-N): " << cfg.num_threads << "\n";
    std::cout << " 允許副檔名(-T)  : " << (cfg.allowed_extensions.empty() ? "所有檔案" : cfg.allowed_extensions) << "\n";
    std::cout << " 大小限制(-S)    : " << (cfg.max_file_size_kb == 0 ? "無限制" : std::to_string(cfg.max_file_size_kb) + " KB") << "\n";
    
    std::cout << "================================\n";
}

#endif // CONFIG_H