#if defined(__APPLE__) || defined(MEGALOPOOL_POSIX_TEST)  // macro de test sous Linux uniquement

#include "Platform.h"

#include <curl/curl.h>
#include <dlfcn.h>

#include <cstdio>
#include <cstdlib>

namespace platform {
namespace {

size_t collect(char* data, size_t size, size_t count, void* userdata) {
    static_cast<std::string*>(userdata)->append(data, size * count);
    return size * count;
}

std::string shellQuote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    return out + "'";
}

} // namespace

HttpResponse httpRequest(const std::string& method, const std::string& url, const std::string& formBody) {
    static const bool initialized = (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
    HttpResponse res;
    if (!initialized) { res.error = "libcurl indisponible"; return res; }

    CURL* curl = curl_easy_init();
    if (!curl) { res.error = "libcurl indisponible"; return res; }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MegaloPool-VirtualDJ/1.0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, collect);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);
    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, formBody.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)formBody.size());
    }

    CURLcode code = curl_easy_perform(curl);
    if (code == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status);
    } else {
        res.error = std::string("Serveur injoignable : ") + curl_easy_strerror(code);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return res;
}

std::string pluginDirectory() {
    // Binaire : .../OnlineSources/MegaloPool.bundle/Contents/MacOS/MegaloPool
    // On remonte jusqu'au dossier qui contient le .bundle.
    Dl_info info{};
    if (!dladdr(reinterpret_cast<void*>(&pluginDirectory), &info) || !info.dli_fname) return "";
    std::string path(info.dli_fname);
    size_t bundle = path.rfind(".bundle/");
    if (bundle != std::string::npos) path = path.substr(0, bundle);
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? "" : path.substr(0, slash + 1);
}

void openUrl(const std::string& url) {
    std::system(("open " + shellQuote(url)).c_str());
}

void openTextFile(const std::string& path) {
    std::system(("open -t " + shellQuote(path)).c_str());
}

bool readFile(const std::string& path, std::string& content) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    char buffer[4096];
    size_t n;
    content.clear();
    while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0) content.append(buffer, n);
    std::fclose(f);
    return true;
}

bool writeFile(const std::string& path, const std::string& content) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = std::fwrite(content.data(), 1, content.size(), f) == content.size();
    std::fclose(f);
    return ok;
}

} // namespace platform

#endif
