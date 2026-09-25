# MegaloPool pour VirtualDJ

Plugin « source en ligne » qui affiche la bibliothèque MegaloPool directement dans le navigateur de VirtualDJ.
Nécessite MegaloPool **v1.42.76** ou plus récent côté serveur.

## Récupérer le plugin compilé

Chaque envoi sur la branche `main` lance la compilation (onglet **Actions** du dépôt, 3 à 5 minutes).
Une fois terminée, ouvrir la dernière exécution et télécharger en bas de page :

- **MegaloPool-Windows** → contient `MegaloPool.dll`
- **MegaloPool-macOS** → contient `MegaloPool-macOS.zip` (le `MegaloPool.bundle`)

Pour une version publiée (onglet **Releases**) : créer un tag `v0.1.0`, par exemple.

## Installation

1. Quitter VirtualDJ.
2. Ouvrir le dossier de VirtualDJ (son emplacement s'affiche dans Paramètres › Options, ligne *homeFolder* ; en général `Documents\VirtualDJ` ou `AppData\Local\VirtualDJ` sous Windows, `Documents/VirtualDJ` sous macOS).
3. Aller dans `Plugins64`, créer le dossier `OnlineSources` s'il n'existe pas.
4. Y copier `MegaloPool.dll` (Windows) ou `MegaloPool.bundle` (macOS, après décompression).
5. **macOS uniquement** : le plugin n'est pas signé par un compte développeur Apple. Dans le Terminal :
   `xattr -dr com.apple.quarantine ~/Documents/VirtualDJ/Plugins64/OnlineSources/MegaloPool.bundle`
   (adapter le chemin si le dossier VirtualDJ est ailleurs).
6. Relancer VirtualDJ : MegaloPool apparaît dans **Online Music**.

## Connexion

Au premier lancement, le plugin crée `MegaloPool.ini` à côté de lui. Cliquer sur MegaloPool puis sur **Connexion** :
le fichier s'ouvre, ainsi que la page Réglages de MegaloPool.

```
server=https://megalo-lab.com
username=votre_identifiant
secret=votre_secret_d_application
lang=fr
```

Le secret d'application se génère dans MegaloPool › Réglages. C'est le même que pour Amperfy : en générer un nouveau
remplace l'ancien, il faudrait alors le reporter aussi dans Amperfy. Enregistrer le fichier, puis cliquer à nouveau sur **Connexion**.

## Utilisation et crédit

- **Pré-écoute gratuite** : clic droit sur un morceau › « Pré-écouter 30 s (gratuit) ».
- **Charger un morceau sur une platine** : le premier chargement d'un morceau l'achète (débit de sa taille), ensuite il est
  gratuit pour toujours, y compris en téléchargement sur le site.
- `✓` dans la colonne Commentaire : morceau déjà acquis.

## Compiler soi-même

CMake 3.20+ ; Visual Studio 2022 (Windows) ou Xcode (macOS).

```
cmake -S . -B build          # Windows : ajouter -A x64
cmake --build build --config Release
```

## Contenu du dépôt

- `src/` : le plugin (`MegaloPoolSource`), le client de l'API (`MegaloPoolApi`) et la couche système (`Platform_win` / `Platform_mac`).
- `src/sdk/` : en-têtes du SDK VirtualDJ (© Atomix Productions), redistribués pour la compilation.
- `src/third_party/json.hpp` : nlohmann/json 3.11.3 (licence MIT).
