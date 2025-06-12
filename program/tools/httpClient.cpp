
//#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "httpClient.h"
#include "../../deps/httplib.h"
#include "../../guikit/api.h"


HttpClient::HttpClient(const std::string& url) {
    this->url = url;
}

auto HttpClient::setProgressCallback(std::function<void(uint64_t len, uint64_t total)> onProgress) -> void {
    this->onProgress = onProgress;
}

auto HttpClient::download(const std::string& uriPath, const std::string& downloadPath) -> bool {

    httplib::Client cli(this->url);

    GUIKIT::File file(downloadPath);

    if (!file.open(GUIKIT::File::Mode::Write, true))
        return false;

    uint64_t offset = 0;

    auto res = cli.Get( uriPath, [&](const char* data, size_t length) {

        if (file.write((const uint8_t*)data, length, offset) != length)
            return false;

        offset += length;
        return true;
        }, [&](uint64_t len, uint64_t total) {
            if (onProgress)
                onProgress(len, total);

            return true;
        }
    );

    if (res && (res->status == httplib::StatusCode::OK_200) )
        return true;

    return false;
}