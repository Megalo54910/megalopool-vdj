// Client de l'API MegaloPool dédiée à VirtualDJ (/api/vdj/v1/).
// Aucune dépendance à VirtualDJ ici : uniquement réseau, JSON et configuration.
#pragma once

#include <mutex>
#include <string>
#include <vector>

struct PluginConfig {
    std::string server;    // ex. https://megalo-lab.com (sans / final)
    std::string username;
    std::string secret;    // secret d'application (Réglages MegaloPool)
    std::string lang = "fr";

    bool complete() const { return !server.empty() && !username.empty() && !secret.empty(); }
};

struct Folder {
    std::string id;
    std::string name;
};

struct Track {
    std::string id, title, artist, genre, style, coverUrl;
    float duration = 0.f;
    float bpm = 0.f;
    int key = 0;       // codage VirtualDJ : 1=Am … 24=G#
    int year = 0;
    bool acquired = false;
};

class MegaloPoolApi {
public:
    // Fichier de configuration, à côté du plugin.
    std::string configPath() const;
    // Crée le fichier modèle s'il n'existe pas. Retourne son chemin.
    std::string ensureConfigFile() const;
    // Relit la configuration. Retourne false si elle est incomplète.
    bool reloadConfig();
    // Efface le secret enregistré (déconnexion).
    void forgetSecret();

    PluginConfig config() const;

    // Vérifie les identifiants auprès du serveur. message : explication en cas d'échec.
    bool checkSession(std::string& message);

    // Dossiers « à plat » : les sous-dossiers sont préfixés par leur rubrique.
    bool listFolders(std::vector<Folder>& out, std::string& message);
    bool folderTracks(const std::string& folderId, std::vector<Track>& out, std::string& message);
    bool search(const std::string& query, std::vector<Track>& out, std::string& message);

    // Chargement sur une platine : premier chargement = achat. url : adresse complète du flux.
    bool play(const std::string& trackId, std::string& url, std::string& message);
    // Extrait gratuit de 30 s. url : adresse complète.
    bool preview(const std::string& trackId, std::string& url, std::string& message);

    std::string absoluteUrl(const std::string& relative) const;

private:
    bool getJson(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params,
                 std::string& body, std::string& message);
    bool postJson(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params,
                  std::string& body, std::string& message);
    bool request(const std::string& method, const std::string& path,
                 std::vector<std::pair<std::string, std::string>> params, std::string& body, std::string& message);
    bool parseTracks(const std::string& body, std::vector<Track>& out, std::string& message) const;

    mutable std::mutex mutex_;
    PluginConfig config_;
};
