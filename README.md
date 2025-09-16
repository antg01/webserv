# webserv
Voici une synthèse claire de **ce qu’il faut livrer** pour *webserv* (campus Bruxelles) et **une découpe judicieuse du travail en équipe de 3**. Pas de code ici—juste l’essentiel pour organiser, construire et tester proprement.

# 1) Ce qu’il faut absolument faire (résumé des exigences)

* **Programme** : `./webserv [configuration file]` (fichier de conf obligatoire ou chemin par défaut).
* **Langage / API** : C++98 uniquement ; I/O réseau non bloquants avec **un seul** mécanisme de multiplexage global : `poll()` ou équivalent (`select`, `kqueue`, `epoll`).

  * **Interdit** : lire/écrire sur un FD sans être d’abord signalé prêt par `poll()` (ou équiv.).
  * **poll/équiv.** doit surveiller **lecture et écriture**.
* **Réseau / Robustesse**

  * Serveur **non bloquant**, gère correctement les **déconnexions**.
  * **Jamais** laisser une requête pendre indéfiniment.
  * **Plusieurs ports** simultanément.
  * Compatible avec **un navigateur standard** (au choix).
* **HTTP**

  * Support **GET, POST, DELETE** (1.0 conseillé comme référence, pas strictement imposé).
  * **Status codes corrects**, **pages d’erreur par défaut** si non fournies.
  * **Site statique** complet servi.
  * **Upload** de fichiers par les clients (emplacement configurable).
* **CGI (au moins un)** : php-cgi, Python, etc.

  * Gérer env vars, passage de la requête complète, **un-chunking** côté serveur avant CGI, **EOF** comme fin si pas de `Content-Length` en sortie CGI.
  * **fork() uniquement pour CGI**.
  * Lancer le CGI dans le **bon répertoire de travail** (accès chemins relatifs).
* **Fichier de configuration** (style NGINX “server/location” simplifié) :

  * Définir `ip:port` multiples.
  * **Pages d’erreur** par défaut ou spécifiques.
  * **client\_max\_body\_size**.
  * Par *location* (pas de regex requis) : méthodes autorisées, redirections, root, autoindex, index par défaut, upload (autorisé + dossier), mapping d’extension → CGI.
* **Qualité**

  * Stress tests ; comparer comportements avec **NGINX** ; tester à **telnet** + navigateur.
  * Fournir **fichiers de conf & jeux de tests** pour la démo.

# 2) Architecture minimale (modules)

* **Core I/O**

  * Accepteurs (listen sockets multi-ports)
  * Boucle d’événements (unique) via poll/équiv.
  * Gestion FD (clients, fichiers, pipes CGI), timeouts
* **HTTP**

  * Parser requêtes (ligne, en-têtes, corps normal ou chunked)
  * Routeur (server/location) → ressource / action
  * Générateur de réponses (status-line, headers, corps ; erreurs par défaut)
* **Config**

  * Parser (format inspiré NGINX) → structures `Server`, `Location`
  * Validation (ports, chemins, directives)
* **Static & Upload**

  * Résolution de chemin (root + path)
  * Listing de dossier (autoindex), index par défaut
  * Gestion upload (vérifier taille, droit, cible)
* **CGI**

  * Préparation env, stdin/out via pipes non bloquants
  * Lancement/attente non bloquants, lecture sortie, normalisation headers
* **Utilitaires**

  * MIME types simples, horodatage HTTP, logger (minimal)

# 3) Division du travail en 3 (lots + interfaces nettes)

## A) Lead **Réseau & Événements** (I/O Core)

**Objectif** : une boucle unique, non bloquante, stable sous charge.

**Responsabilités**

* Sockets d’écoute multi-ports (bind/listen) ; accept non bloquant.
* Boucle `poll()/select/epoll/kqueue` unique qui gère **lecture & écriture** pour :

  * connexions client
  * fichiers (static/ upload)
  * pipes CGI
* Gestion des **timeouts** requêtes (pas de pendaison).
* États de connexion (read headers/body, write response, close).
* Petits buffers réutilisables, backpressure (ne pas tout charger en RAM).
* API vers HTTP & CGI :

  * *callbacks* ou file de messages :

    * **onReadable(fd)** → `HTTP::ingest(fd, bytes)`
    * **wantWrite(fd, bufferView)** → planifier écriture
    * **spawnCGI(cmd, env, cwd, inBuf)** → renvoie `{stdinFD, stdoutFD, pid}` enregistrés dans le poller.

**Livrables**

* Gestionnaire d’événements + table des FDs (type, rôle, deadlines).
* Tests : échos simples, accept multi-ports, simulation d’écritures partielles, timeouts.

---

## B) Lead **HTTP & CGI**

**Objectif** : conformité HTTP (1.0/1.1 raisonnable), méthodes, chunked, CGI.

**Responsabilités**

* **Parser requêtes** tolérant : ligne requête, en-têtes, normalisation, corps :

  * `Content-Length` vs **chunked** (déchunking).
* **Router** en fonction de la conf (server/location).
* **Méthodes** :

  * GET : fichier / autoindex / redirection.
  * POST : upload (respect taille max), ou passage au CGI.
  * DELETE : suppression fichier si autorisé.
* **Réponses** : status-line correcte, en-têtes (`Date`, `Server`, `Content-Length`/`Transfer-Encoding`, `Connection`, `Content-Type`), pages d’erreur par défaut.
* **CGI** :

  * Construction ENV (REQUEST\_METHOD, PATH\_INFO, QUERY\_STRING, CONTENT\_\* …).
  * Un-chunk du corps avant envoi au CGI ; EOF marque fin si pas de `Content-Length` en sortie.
  * Lancement via API du lot A ; lecture non bloquante sortie CGI ; normalisation des headers CGI → HTTP.
* **Compat navigateurs** : persistance simple (close après réponse ou keep-alive basique si vous voulez).

**Livrables**

* Parser robuste + générateur réponses.
* Implémentation CGI (au moins 1, ex. php-cgi ou python).
* Jeux d’essais : requêtes brutes (telnet), fichiers de test (gros/ chunked), scripts CGI de démo.

---

## C) Lead **Configuration, Fichiers statiques & Outils**

**Objectif** : conf inspirée NGINX, static server complet, DX (dev experience).

**Responsabilités**

* **Parser de configuration** (fichier → structures) :

  * `server` : `listen ip:port`, `error_page`, `client_max_body_size`, `server_name` (optionnel).
  * `location` : `root`, `methods`, `return` (redirection), `autoindex on|off`, `index`, `upload_store`, `cgi_pass` par extension.
* **Validation** (ports valides, dossiers existants, valeurs cohérentes).
* **Fichiers statiques** :

  * Résolution de chemin sûre (éviter `..`), `index` si répertoire, **autoindex** HTML simple.
  * **MIME types** basiques (table embarquée).
  * **Uploads** : créer dossier cible si nécessaire, respecter `client_max_body_size`.
* **Assets projet** :

  * **Makefile** (cibles demandées), arbo propre (`src/`, `include/`, `conf/`, `www/`, `cgi-bin/`, `tests/`).
  * **Fichiers de conf d’exemple** couvrant tous les cas.
  * **Pages d’erreur par défaut** (40x, 50x).
  * **Scripts de test** (Python/Golang simple) pour smoke/stress.

**Livrables**

* Parser conf + structures, autoindex + index, uploads.
* Dossiers & contenus par défaut pour l’évaluation (site statique, CGI de démo, confs).

# 4) Plan d’intégration (proposé, 2–3 semaines)

* **Jours 1–2**

  * C : Parser de conf minimal (un `server` + un `location`, `listen`, `root`).
  * A : Sockets d’écoute multi-ports + boucle poll, accept/close, echo server.
  * B : Parser ligne de requête + en-têtes (sans corps encore), réponse 200 “Hello”.

* **Jours 3–5**

  * B : Corps `Content-Length`, réponses de fichiers (GET), erreurs par défaut.
  * C : Autoindex, MIME types, index par défaut.
  * A : Timeouts & écriture partielle, backpressure.

* **Jours 6–8**

  * B : **chunked** (déchunk), **POST upload** (via C), **DELETE**.
  * C : `client_max_body_size`, contrôle méthodes/403, redirections.
  * A : Nettoyage FDs, scénarios déconnexion en lecture/écriture.

* **Jours 9–11**

  * B : **CGI** (env, cwd, pipes non bloquants, sortie sans `Content-Length` → EOF).
  * C : Mapping extension→CGI dans conf ; fournir scripts CGI de test.
  * A : Gestion `fork` limité au CGI, `waitpid` non bloquant.

* **Jours 12–14**

  * **Compat navigateur** (Chrome/Firefox), tests telnet, comparaison NGINX.
  * **Stress tests** (concurrence, gros fichiers, lenteurs réseau simulées).
  * Finition confs, pages d’erreur, doc rapide “comment tester”.

# 5) Interfaces claires entre lots (contrats)

* **Conf → HTTP/Routeur** :

  * `Server{ vector<Location>, vector<Listen>, error_pages, max_body, … }`
  * `Location{ path_prefix, root, methods, autoindex, index, upload_dir, cgi_map, redirect }`
* **HTTP ↔ I/O** :

  * `scheduleWrite(fd, span)` ; `onReadable(fd, span)` ; `close(fd)` ; `setTimeout(fd, tms)`
* **HTTP ↔ CGI** :

  * `pid = spawnCGI(cmd, env, cwd, inFD/outFD)` ; `onCGIStdout(fd, span)` ; `onCGIExit(pid, status)`

# 6) Stratégie de tests

* **Unitaires** : parser conf, parser HTTP (lignes tordues, headers dupliqués, chunked invalide), normalisation chemins, limites taille corps.
* **Intégration** :

  * GET fichier/404/403, autoindex, index.
  * POST upload (limite taille, dossier), DELETE (autorisations).
  * CGI : entrée + sortie sans `Content-Length`, variables d’env, cwd.
* **E2E** : curl/wget/telnet + navigateur ; comparer entêtes avec NGINX.
* **Stress** : petit script Python (threads/async) qui ouvre N connexions, envoie headers lents, uploads, annule en plein milieu, etc.

# 7) Check-list conformité (rapide)

* [ ] Un seul `poll()/select/epoll/kqueue` pour TOUT (listen inclus) & lecture/écriture.
* [ ] Aucune lecture/écriture sans évènement prêt.
* [ ] Timeouts → aucune requête ne pend.
* [ ] Multi-ports OK.
* [ ] GET/POST/DELETE OK.
* [ ] Uploads (emplacement configurable).
* [ ] CGI (au moins 1), unchunk avant, EOF si pas de `Content-Length`.
* [ ] Pages d’erreur par défaut.
* [ ] Site statique complet servi, autoindex optionnel.
* [ ] Fichier de conf conforme (méthodes, redirs, root, index, client\_max\_body\_size, cgi by ext).
* [ ] Compatible navigateur ; comparaison NGINX.
* [ ] Fichiers de démo fournis (conf + www + cgi + erreurs).
* [ ] Makefile : `NAME, all, clean, fclean, re`.

# 8) Pièges fréquents (à éviter)

* Lire/écrire sans signal du poller → **0 direct**.
* Oublier la **gestion d’écritures partielles** (send ne vide pas tout).
* **Chunked** mal géré (espaces, extensions chunk, CRLF).
* `client_max_body_size` ignoré pour uploads et CGI.
* **Bloquer** sur CGI (pipes ou `waitpid`).
* Résolution de chemin non sécurisée (`..`), autoindex listant trop.
* FDs non fermés → fuite + épuisement poller.

---

Si tu veux, je peux te générer :

* un **squelette d’arborescence** de projet (src/include/conf/www/cgi-bin/tests),
* un **exemple de fichier de conf** couvrant toutes les directives,
* une **checklist de tests curl/telnet** prête à copier-coller,
* ou un **mini-plan de classes** (≤25 lignes par fonction) conforme à tes règles.
