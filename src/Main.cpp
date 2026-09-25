// Point d'entrée chargé par VirtualDJ (modèle COM du SDK).
#include "MegaloPoolSource.h"

#include <cstring>

HRESULT VDJ_API DllGetClassObject(const GUID& rclsid, const GUID& riid, void** ppObject) {
    if (std::memcmp(&rclsid, &CLSID_VdjPlugin8, sizeof(GUID)) != 0) return CLASS_E_CLASSNOTAVAILABLE;
    // Selon la version de VirtualDJ, une source en ligne est demandée via son
    // interface dédiée ou via l'interface de base : les deux sont acceptées.
    if (std::memcmp(&riid, &IID_IVdjPluginOnlineSource, sizeof(GUID)) != 0 &&
        std::memcmp(&riid, &IID_IVdjPluginBasic8, sizeof(GUID)) != 0) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    *ppObject = new MegaloPoolSource();
    return NO_ERROR;
}
