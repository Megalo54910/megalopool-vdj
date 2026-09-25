// Source en ligne « MegaloPool » pour VirtualDJ 8+.
#pragma once

#include "sdk/vdjOnlineSource.h"
#include "MegaloPoolApi.h"

#include <atomic>

#ifndef MEGALOPOOL_PLUGIN_VERSION
#define MEGALOPOOL_PLUGIN_VERSION "0.1.0"  // remplacé par le fichier VERSION à la compilation
#endif

class MegaloPoolSource : public IVdjPluginOnlineSource {
public:
    HRESULT VDJ_API OnLoad() override;
    HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8* infos) override;

    HRESULT VDJ_API IsLogged() override;
    HRESULT VDJ_API OnLogin() override;
    HRESULT VDJ_API OnLogout() override;

    HRESULT VDJ_API OnSearch(const char* search, IVdjTracksList* tracksList) override;
    HRESULT VDJ_API OnSearchCancel() override;
    HRESULT VDJ_API GetStreamUrl(const char* uniqueId, IVdjString& url, IVdjString& errorMessage) override;

    HRESULT VDJ_API GetFolderList(IVdjSubfoldersList* subfoldersList) override;
    HRESULT VDJ_API GetFolder(const char* folderUniqueId, IVdjTracksList* tracksList) override;

    HRESULT VDJ_API GetContextMenu(const char* uniqueId, IVdjContextMenu* contextMenu) override;
    HRESULT VDJ_API OnContextMenu(const char* uniqueId, size_t menuIndex) override;

private:
    void addTracks(const std::vector<Track>& tracks, IVdjTracksList* list);
    bool ensureLogged();

    MegaloPoolApi api_;
    std::atomic<bool> logged_{false};
    std::atomic<bool> searchCancelled_{false};
};
