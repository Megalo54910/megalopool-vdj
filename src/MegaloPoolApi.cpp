#include "MegaloPoolApi.h"
#include "Platform.h"

#include "third_party/json.hpp"

#include <cctype>
#include <cstdio>
#include <sstream>

using json = nlohmann::json;

namespace {

const char* kConfigFileName = "MegaloPool.ini";
const char* kApiPrefix = "/api/vdj/v1";

const char* kConfigTemplate =
    "; Configuration du plugin MegaloPool pour VirtualDJ\n"
    "; 1. server   : adresse de MegaloPool, sans / final (ex. https://megalo-lab.com)\n"
    "; 2. username : votre identifiant MegaloPool\n"
    "; 3. secret   : votre secret d'application, a generer dans MegaloPool > Reglages\n"
    ";               (le meme que pour Amperfy ou une autre appli Subsonic)\n"
    "; 4. lang     : fr, en ou es\n"
    "; Enregistrez ce fichier, puis cliquez a nouveau sur Connexion dans VirtualDJ.\n"
    "server=https://megalo-lab.com\n"
    "username=\n"
    "secret=\n"
    "lang=fr\n";

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string urlEncode(const std::string& value) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

std::string encodeParams(const std::vector<std::pair<std::string, std::string>>& params) {
    std::string out;
    for (const auto& p : params) {
        if (!out.empty()) out += '&';
        out += urlEncode(p.first) + "=" + urlEncode(p.second);
    }
    return out;
}

std::string str(const json& obj, const char* key) {
    auto it = obj.find(key);
    if (it == obj.end() || it->is_null()) return "";
    if (it->is_string()) return it->get<std::string>();
    return it->dump();
}

double num(const json& obj, const char* key) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_number()) return 0.0;
    return it->get<double>();
}

std::string serverMessage(const std::string& body) {
    json doc = json::parse(body, nullptr, false);
    if (doc.is_object()) return str(doc, "message");
    return "";
}

} // namespace

std::string MegaloPoolApi::configPath() const {
    return platform::pluginDirectory() + kConfigFileName;
}

std::string MegaloPoolApi::ensureConfigFile() const {
    std::string path = configPath();
    std::string existing;
    if (!platform::readFile(path, existing)) platform::writeFile(path, kConfigTemplate);
    return path;
}

bool MegaloPoolApi::reloadConfig() {
    PluginConfig cfg;
    std::string content;
    if (platform::readFile(configPath(), content)) {
        std::istringstream lines(content);
        std::string line;
        while (std::getline(lines, line)) {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = trim(line.substr(0, eq));
            std::string value = trim(line.substr(eq + 1));
            if (key == "server") {
                while (!value.empty() && value.back() == '/') value.pop_back();
                cfg.server = value;
            } else if (key == "username") {
                cfg.username = value;
            } else if (key == "secret") {
                cfg.secret = value;
            } else if (key == "lang" && !value.empty()) {
                cfg.lang = value;
            }
        }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = cfg;
    return config_.complete();
}

void MegaloPoolApi::forgetSecret() {
    std::string content;
    if (!platform::readFile(configPath(), content)) return;
    std::istringstream lines(content);
    std::string line, out;
    while (std::getline(lines, line)) {
        std::string t = trim(line);
        if (t.rfind("secret", 0) == 0 && t.find('=') != std::string::npos) line = "secret=";
        out += line + "\n";
    }
    platform::writeFile(configPath(), out);
    std::lock_guard<std::mutex> lock(mutex_);
    config_.secret.clear();
}

PluginConfig MegaloPoolApi::config() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

std::string MegaloPoolApi::absoluteUrl(const std::string& relative) const {
    if (relative.empty() || relative.rfind("http", 0) == 0) return relative;
    return config().server + relative;
}

bool MegaloPoolApi::request(const std::string& method, const std::string& path,
                            std::vector<std::pair<std::string, std::string>> params,
                            std::string& body, std::string& message) {
    PluginConfig cfg = config();
    if (!cfg.complete()) {
        message = "Plugin non configure : completez MegaloPool.ini (Connexion).";
        return false;
    }
    params.emplace_back("u", cfg.username);
    params.emplace_back("p", cfg.secret);
    params.emplace_back("lang", cfg.lang);
    std::string encoded = encodeParams(params);
    std::string url = cfg.server + kApiPrefix + path;

    platform::HttpResponse res = method == "POST"
        ? platform::httpRequest("POST", url, encoded)
        : platform::httpRequest("GET", url + "?" + encoded);

    if (res.status == 0) {
        message = res.error.empty() ? "Serveur MegaloPool injoignable." : res.error;
        return false;
    }
    body = res.body;
    if (res.status == 401) {
        message = "Identifiant ou secret d'application refuse : verifiez MegaloPool.ini.";
        return false;
    }
    if (res.status >= 300) {
        message = serverMessage(res.body);
        if (message.empty()) message = "Erreur du serveur MegaloPool (code " + std::to_string(res.status) + ").";
        return false;
    }
    return true;
}

bool MegaloPoolApi::getJson(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params,
                            std::string& body, std::string& message) {
    return request("GET", path, params, body, message);
}

bool MegaloPoolApi::postJson(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params,
                             std::string& body, std::string& message) {
    return request("POST", path, params, body, message);
}

bool MegaloPoolApi::checkSession(std::string& message) {
    std::string body;
    return getJson("/session", {}, body, message);
}

bool MegaloPoolApi::listFolders(std::vector<Folder>& out, std::string& message) {
    out.clear();
    std::string body;
    if (!getJson("/folders", {}, body, message)) return false;
    json root = json::parse(body, nullptr, false);
    if (!root.is_object() || !root["folders"].is_array()) {
        message = "Reponse inattendue du serveur (dossiers).";
        return false;
    }
    for (const json& folder : root["folders"]) {
        const std::string id = str(folder, "id");
        const std::string name = str(folder, "name");
        auto hasChildren = folder.find("has_children");
        if (hasChildren == folder.end() || !hasChildren->is_boolean() || !hasChildren->get<bool>()) {
            out.push_back({id, name});
            continue;
        }
        std::string childBody, childMessage;
        if (!getJson("/folders", {{"parent", id}}, childBody, childMessage)) continue;
        json children = json::parse(childBody, nullptr, false);
        if (!children.is_object() || !children["folders"].is_array()) continue;
        for (const json& child : children["folders"]) {
            out.push_back({str(child, "id"), name + " \xE2\x80\xBA " + str(child, "name")});  // « › »
        }
    }
    return true;
}

bool MegaloPoolApi::parseTracks(const std::string& body, std::vector<Track>& out, std::string& message) const {
    out.clear();
    json root = json::parse(body, nullptr, false);
    if (!root.is_object() || !root["tracks"].is_array()) {
        message = "Reponse inattendue du serveur (morceaux).";
        return false;
    }
    out.reserve(root["tracks"].size());
    for (const json& item : root["tracks"]) {
        Track t;
        t.id = str(item, "id");
        t.title = str(item, "title");
        t.artist = str(item, "artist");
        t.genre = str(item, "genre");
        t.style = str(item, "style");
        t.album = str(item, "album");
        t.coverUrl = absoluteUrl(str(item, "cover_url"));
        t.duration = (float)num(item, "duration");
        t.bpm = (float)num(item, "bpm");
        t.key = (int)num(item, "key");
        t.year = (int)num(item, "year");
        auto acquired = item.find("acquired");
        t.acquired = acquired != item.end() && acquired->is_boolean() && acquired->get<bool>();
        if (!t.id.empty()) out.push_back(std::move(t));
    }
    return true;
}

bool MegaloPoolApi::folderTracks(const std::string& folderId, std::vector<Track>& out, std::string& message) {
    std::string body;
    return getJson("/tracks", {{"folder", folderId}}, body, message) && parseTracks(body, out, message);
}

bool MegaloPoolApi::search(const std::string& query, std::vector<Track>& out, std::string& message) {
    std::string body;
    return getJson("/search", {{"q", query}}, body, message) && parseTracks(body, out, message);
}

bool MegaloPoolApi::play(const std::string& trackId, std::string& url, std::string& message) {
    std::string body;
    if (!postJson("/play", {{"id", trackId}}, body, message)) return false;
    json root = json::parse(body, nullptr, false);
    url = root.is_object() ? absoluteUrl(str(root, "url")) : "";
    if (url.empty()) { message = "Reponse inattendue du serveur (lecture)."; return false; }
    return true;
}

bool MegaloPoolApi::preview(const std::string& trackId, std::string& url, std::string& message) {
    std::string body;
    if (!getJson("/preview", {{"id", trackId}}, body, message)) return false;
    json root = json::parse(body, nullptr, false);
    url = root.is_object() ? absoluteUrl(str(root, "url")) : "";
    if (url.empty()) { message = "Reponse inattendue du serveur (pre-ecoute)."; return false; }
    return true;
}
