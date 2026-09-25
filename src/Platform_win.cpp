#ifdef _WIN32

#include "Platform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>

#include <cstdio>
#include <vector>

namespace platform {
namespace {

std::wstring widen(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &out[0], n);
    return out;
}

std::string narrow(const std::wstring& s) {
    if (s.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), &out[0], n, nullptr, nullptr);
    return out;
}

struct Handle {
    HINTERNET h = nullptr;
    explicit Handle(HINTERNET v) : h(v) {}
    ~Handle() { if (h) WinHttpCloseHandle(h); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};

} // namespace

HttpResponse httpRequest(const std::string& method, const std::string& url, const std::string& formBody) {
    HttpResponse res;
    std::wstring wurl = widen(url);

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t host[256] = {0};
    wchar_t path[4096] = {0};
    parts.lpszHostName = host;
    parts.dwHostNameLength = 255;
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = 4095;
    wchar_t extra[4096] = {0};
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = 4095;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &parts)) {
        res.error = "Adresse du serveur invalide";
        return res;
    }
    std::wstring target = std::wstring(path) + extra;
    bool secure = parts.nScheme == INTERNET_SCHEME_HTTPS;

    Handle session(WinHttpOpen(L"MegaloPool-VirtualDJ/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.h) { res.error = "WinHTTP indisponible"; return res; }
    WinHttpSetTimeouts(session.h, 10000, 15000, 30000, 60000);

    Handle connect(WinHttpConnect(session.h, host, parts.nPort, 0));
    if (!connect.h) { res.error = "Connexion impossible"; return res; }

    Handle request(WinHttpOpenRequest(connect.h, widen(method).c_str(), target.c_str(), nullptr,
                                      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                      secure ? WINHTTP_FLAG_SECURE : 0));
    if (!request.h) { res.error = "Requête impossible"; return res; }

    const wchar_t* headers = L"Content-Type: application/x-www-form-urlencoded\r\nAccept: application/json\r\n";
    BOOL sent = WinHttpSendRequest(request.h, headers, (DWORD)-1L,
                                   formBody.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)formBody.data(),
                                   (DWORD)formBody.size(), (DWORD)formBody.size(), 0);
    if (!sent || !WinHttpReceiveResponse(request.h, nullptr)) {
        res.error = "Serveur injoignable";
        return res;
    }

    DWORD status = 0, size = sizeof(status);
    WinHttpQueryHeaders(request.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
    res.status = (long)status;

    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.h, &available) || available == 0) break;
        std::vector<char> buffer(available);
        DWORD read = 0;
        if (!WinHttpReadData(request.h, buffer.data(), available, &read) || read == 0) break;
        res.body.append(buffer.data(), read);
    }
    return res;
}

std::string pluginDirectory() {
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&pluginDirectory), &module);
    wchar_t buffer[MAX_PATH] = {0};
    GetModuleFileNameW(module, buffer, MAX_PATH);
    std::wstring full(buffer);
    size_t slash = full.find_last_of(L"\\/");
    return narrow(slash == std::wstring::npos ? L"" : full.substr(0, slash + 1));
}

void openUrl(const std::string& url) {
    ShellExecuteW(nullptr, L"open", widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void openTextFile(const std::string& path) {
    ShellExecuteW(nullptr, L"open", L"notepad.exe", widen("\"" + path + "\"").c_str(), nullptr, SW_SHOWNORMAL);
}

bool readFile(const std::string& path, std::string& content) {
    FILE* f = _wfopen(widen(path).c_str(), L"rb");
    if (!f) return false;
    char buffer[4096];
    size_t n;
    content.clear();
    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0) content.append(buffer, n);
    fclose(f);
    return true;
}

bool writeFile(const std::string& path, const std::string& content) {
    FILE* f = _wfopen(widen(path).c_str(), L"wb");
    if (!f) return false;
    bool ok = fwrite(content.data(), 1, content.size(), f) == content.size();
    fclose(f);
    return ok;
}

} // namespace platform

#endif
