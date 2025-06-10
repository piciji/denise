
#pragma once

#include <string>
#include <functional>
#include <cstdint>

struct HttpClient {

    std::string url;

    std::function<void(uint64_t len, uint64_t total)> onProgress = nullptr;

    HttpClient(const std::string& url);

    auto setProgressCallback(std::function<void(uint64_t len, uint64_t total)> onProgress) -> void;

    auto download(const std::string& uriPath, const std::string& downloadPath) -> bool;
};
