#include "MegaloPoolSource.h"
#include "Platform.h"

#include <cstring>
#include <string>

namespace {
const char* kMenuPreview = "Pre-ecouter 30 s (gratuit)";
const char* kCheckMark = "\xE2\x9C\x93";  // « ✓ » : morceau déjà acquis, rechargement gratuit
} // namespace

HRESULT VDJ_API MegaloPoolSource::OnLoad() {
    api_.ensureConfigFile();
    api_.reloadConfig();
    return S_OK;
}

HRESULT VDJ_API MegaloPoolSource::OnGetPluginInfo(TVdjPluginInfo8* infos) {
    infos->PluginName = "MegaloPool";
    infos->Author = "megalo-lab";
    infos->Description = "Parcourir et jouer la bibliotheque MegaloPool";
    infos->Version = MEGALOPOOL_PLUGIN_VERSION;
    infos->Flags = 0;
    infos->Bitmap = nullptr;
    return S_OK;
}

// ------------------------------------------------------------------ connexion
bool MegaloPoolSource::ensureLogged() {
    if (logged_) return true;
    if (!api_.reloadConfig()) return false;
    std::string message;
    logged_ = api_.checkSession(message);
    return logged_;
}

HRESULT VDJ_API MegaloPoolSource::IsLogged() {
    return ensureLogged() ? S_OK : S_FALSE;
}

HRESULT VDJ_API MegaloPoolSource::OnLogin() {
    logged_ = false;
    if (ensureLogged()) return S_OK;
    // Configuration absente ou refusée : on ouvre le fichier à compléter et la
    // page Réglages de MegaloPool, où se génère le secret d'application.
    std::string path = api_.ensureConfigFile();
    PluginConfig cfg = api_.config();
    platform::openTextFile(path);
    if (!cfg.server.empty()) platform::openUrl(cfg.server + "/settings");
    return S_OK;
}

HRESULT VDJ_API MegaloPoolSource::OnLogout() {
    api_.forgetSecret();
    logged_ = false;
    return S_OK;
}

// ------------------------------------------------------------------ morceaux
void MegaloPoolSource::addTracks(const std::vector<Track>& tracks, IVdjTracksList* list) {
    for (const Track& t : tracks) {
        const char* comment = t.acquired ? kCheckMark : "";
        list->add(t.id.c_str(), t.title.c_str(), t.artist.c_str(),
                  nullptr,                       // remix
                  t.genre.c_str(),
                  nullptr,                       // label : inconnu (pas de détournement de colonne)
                  comment,
                  t.coverUrl.empty() ? nullptr : t.coverUrl.c_str(),
                  nullptr,                       // flux fourni à la demande (GetStreamUrl)
                  t.duration, t.bpm, t.key, t.year,
                  false, false);
    }
}

HRESULT VDJ_API MegaloPoolSource::OnSearch(const char* search, IVdjTracksList* tracksList) {
    searchCancelled_ = false;
    if (!search || !*search || !ensureLogged()) return S_OK;
    std::vector<Track> tracks;
    std::string message;
    if (api_.search(search, tracks, message) && !searchCancelled_) addTracks(tracks, tracksList);
    return S_OK;
}

HRESULT VDJ_API MegaloPoolSource::OnSearchCancel() {
    searchCancelled_ = true;
    return S_OK;
}

HRESULT VDJ_API MegaloPoolSource::GetStreamUrl(const char* uniqueId, IVdjString& url, IVdjString& errorMessage) {
    if (!ensureLogged()) {
        errorMessage = "MegaloPool : connectez-vous d'abord (MegaloPool.ini).";
        return S_FALSE;
    }
    std::string streamUrl, message;
    if (!api_.play(uniqueId ? uniqueId : "", streamUrl, message)) {
        errorMessage = message.c_str();
        return S_FALSE;
    }
    url = streamUrl.c_str();
    return S_OK;
}

// ------------------------------------------------------------------ dossiers
HRESULT VDJ_API MegaloPoolSource::GetFolderList(IVdjSubfoldersList* subfoldersList) {
    if (!ensureLogged()) return S_OK;
    std::vector<Folder> folders;
    std::string message;
    if (!api_.listFolders(folders, message)) return S_OK;
    for (const Folder& f : folders) subfoldersList->add(f.id.c_str(), f.name.c_str());
    return S_OK;
}

HRESULT VDJ_API MegaloPoolSource::GetFolder(const char* folderUniqueId, IVdjTracksList* tracksList) {
    if (!folderUniqueId || !ensureLogged()) return S_OK;
    std::vector<Track> tracks;
    std::string message;
    if (api_.folderTracks(folderUniqueId, tracks, message)) addTracks(tracks, tracksList);
    return S_OK;
}

// ------------------------------------------------------------------ menus
HRESULT VDJ_API MegaloPoolSource::GetContextMenu(const char* /*uniqueId*/, IVdjContextMenu* contextMenu) {
    contextMenu->add(kMenuPreview);
    return S_OK;
}

HRESULT VDJ_API MegaloPoolSource::OnContextMenu(const char* uniqueId, size_t menuIndex) {
    if (menuIndex != 0 || !uniqueId || !ensureLogged()) return S_OK;
    std::string url, message;
    if (api_.preview(uniqueId, url, message)) platform::openUrl(url);
    return S_OK;
}
