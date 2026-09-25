// Couche plateforme : tout ce qui diffère entre Windows et macOS.
// Windows : WinHTTP + Shell (aucune dépendance à livrer).
// macOS   : libcurl du système + CoreFoundation.
#pragma once

#include <string>

namespace platform {

struct HttpResponse {
    long status = 0;        // 0 = échec réseau (voir error)
    std::string body;
    std::string error;
};

// method : "GET" ou "POST". formBody : corps application/x-www-form-urlencoded (POST).
HttpResponse httpRequest(const std::string& method, const std::string& url, const std::string& formBody = "");

// Dossier qui contient le plugin (.dll ou .bundle), terminé par un séparateur.
std::string pluginDirectory();

// Ouvre une adresse dans le navigateur par défaut.
void openUrl(const std::string& url);

// Ouvre un fichier texte dans l'éditeur par défaut.
void openTextFile(const std::string& path);

// Lecture/écriture d'un fichier entier (chemins UTF-8, y compris accentués).
bool readFile(const std::string& path, std::string& content);
bool writeFile(const std::string& path, const std::string& content);

} // namespace platform
