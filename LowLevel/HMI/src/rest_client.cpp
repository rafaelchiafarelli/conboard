#include "rest_client.hpp"

#include <curl/curl.h>

#include <iostream>

namespace {
size_t writeCb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto *out = static_cast<std::string *>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

// curl_global_init/cleanup are process-wide and not thread-safe to call
// concurrently; conHMI is single-process, so a static guard here keeps every
// RestClient caller from having to remember to call it.
struct CurlGlobal {
    CurlGlobal() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
};
}  // namespace

RestClient::RestClient(std::string baseUrl, std::string user, std::string pswdHash)
    : baseUrl_(std::move(baseUrl)), user_(std::move(user)), pswdHash_(std::move(pswdHash))
{
    static CurlGlobal guard;
}

std::optional<nlohmann::json> RestClient::get(const std::string &path) const
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "RestClient: curl_easy_init failed" << std::endl;
        return std::nullopt;
    }

    const std::string url = baseUrl_ + path;
    std::string body;

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, ("X-User: " + user_).c_str());
    headers = curl_slist_append(headers, ("X-Pswd: " + pswdHash_).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);

    CURLcode res = curl_easy_perform(curl);
    long httpStatus = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "RestClient: GET " << url << " failed: " << curl_easy_strerror(res) << std::endl;
        return std::nullopt;
    }
    if (httpStatus < 200 || httpStatus >= 300) {
        std::cerr << "RestClient: GET " << url << " -> HTTP " << httpStatus << std::endl;
        return std::nullopt;
    }

    try {
        return nlohmann::json::parse(body);
    } catch (const nlohmann::json::parse_error &e) {
        std::cerr << "RestClient: GET " << url << " returned invalid JSON: " << e.what() << std::endl;
        return std::nullopt;
    }
}
