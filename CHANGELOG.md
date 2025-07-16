# Changelog

Toutes les modifications notables de ce projet seront documentées dans ce fichier.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/),
et ce projet adhère au [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-12-19

### Ajouté
- **Architecture modulaire complète** : Séparation claire des responsabilités
  - `CameraManager` : Gestion de la caméra ESP32-CAM
  - `WiFiManager` : Gestion WiFi robuste avec monitoring
  - `NanoRTSPServer` : Serveur RTSP MJPEG multi-clients
  - `HTTPMJPEGServer` : Serveur HTTP MJPEG
  - `Utils/Logger` : Système de logging professionnel
  - `Utils/Helpers` : Fonctions utilitaires

- **Configuration 100% centralisée** : Tous les paramètres dans `config.h`
  - Aucune valeur hardcodée dans le code source
  - Configuration WiFi, caméra, RTSP, HTTP, etc.
  - Type de callback universel `CaptureCallback`

- **Serveur RTSP MJPEG multi-clients** :
  - Support jusqu'à 5 clients simultanés
  - Sessions RTSP indépendantes par client
  - Gestion automatique des connexions/déconnexions
  - Numéros de séquence RTP uniques par session

- **Serveur HTTP MJPEG** :
  - Accès direct via navigateur web
  - Stream MJPEG natif
  - Compatible avec tous les navigateurs

- **Gestion WiFi robuste** :
  - Connexion avec retry automatique
  - Monitoring de la qualité du signal
  - Reconnexion automatique en cas de déconnexion
  - Diagnostic et logs détaillés

- **Logger professionnel** :
  - 5 niveaux de verbosité (ERROR, WARN, INFO, DEBUG, VERBOSE)
  - Formatage avec timestamps
  - Macros facilitant l'utilisation
  - Configuration dynamique du niveau

- **Gestion mémoire optimisée** :
  - Libération automatique des frame buffers
  - Monitoring de l'utilisation mémoire
  - Code non-bloquant avec `yield()`

- **Documentation complète** :
  - README détaillé avec instructions d'installation
  - FAQ et dépannage
  - Documentation des headers avec exemples
  - Configuration PlatformIO optimisée

### Modifié
- Refactoring complet de l'architecture
- Centralisation de tous les paramètres
- Amélioration de la stabilité et des performances

### Corrigé
- Gestion mémoire pour éviter les fuites
- Synchronisation des accès à la caméra
- Logs et diagnostics améliorés

## [0.9.0] - 2024-12-18

### Ajouté
- Version initiale du firmware
- Support RTSP basique
- Configuration caméra ESP32-CAM

---

## Types de modifications

- **Ajouté** : Nouvelles fonctionnalités
- **Modifié** : Changements dans les fonctionnalités existantes
- **Déprécié** : Fonctionnalités qui seront supprimées
- **Supprimé** : Fonctionnalités supprimées
- **Corrigé** : Corrections de bugs
- **Sécurité** : Corrections de vulnérabilités 