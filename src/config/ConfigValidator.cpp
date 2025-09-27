#include "config/ConfigValidator.hpp"
#include "config/ServerBlock.hpp"
#include "config/LocationBlock.hpp"
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <sstream>

// La fonction toStringSizeT convertit une valeur numérique de type size_t en chaîne de caractères.
// - Elle crée un flux de sortie (ostringstream).
// - Elle insère la valeur n dans ce flux.
// - Elle retourne la représentation sous forme de std::string.
// → C’est une fonction utilitaire pour transformer un entier en texte.
static std::string toStringSizeT(size_t n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

// La fonction isValidPort vérifie si un entier correspond à un port réseau valide.
// - Un port est considéré valide s’il est compris entre 1 et 65535 inclus.
// - Elle retourne true si p est dans cette plage, sinon false.
// → Utile pour s’assurer qu’une configuration n’utilise pas un port invalide.
bool ConfigValidator::isValidPort(int p)
{
    return p >= 1 && p <= 65535;
}

// La fonction pathExists vérifie si un chemin de fichier ou de répertoire existe.
// - Elle déclare une structure stat pour stocker les informations sur le fichier.
// - Elle appelle la fonction système stat() avec le chemin p.
// - Si stat() retourne 0, cela signifie que le chemin existe → la fonction retourne true.
// - Sinon, le chemin n’existe pas ou n’est pas accessible → la fonction retourne false.
// → Utile pour valider qu’un fichier ou dossier mentionné dans la configuration est bien présent.
bool ConfigValidator::pathExists(const std::string &p)
{
    struct stat st;
    return ::stat(p.c_str(), &st) == 0;
}

// La fonction isDir vérifie si un chemin correspond à un répertoire.
// - Elle déclare une structure stat pour récupérer les informations du chemin.
// - Elle appelle la fonction système stat() avec le chemin p.
//   * Si stat() échoue (valeur de retour différente de 0), elle retourne false.
// - Sinon, elle teste le champ st_mode avec la macro S_ISDIR() :
//   * retourne true si c’est un répertoire,
//   * false dans le cas contraire.
// → Utile pour s’assurer qu’un chemin de configuration pointe bien vers un dossier.
bool ConfigValidator::isDir(const std::string &p)
{
    struct stat st;
    if (::stat(p.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode) ? true : false;
}

// La fonction isFile vérifie si un chemin correspond à un fichier régulier.
// - Elle déclare une structure stat pour récupérer les informations sur le chemin.
// - Elle appelle la fonction système stat() avec le chemin p.
//   * Si stat() échoue (valeur de retour différente de 0), elle retourne false.
// - Sinon, elle teste le champ st_mode avec la macro S_ISREG() :
//   * retourne true si le chemin correspond à un fichier régulier,
//   * false dans le cas contraire (répertoire, lien, périphérique, etc.).
// → Utile pour s’assurer qu’un chemin de configuration pointe bien vers un fichier normal.
bool ConfigValidator::isFile(const std::string &p)
{
    struct stat st;
    if (::stat(p.c_str(), &st) != 0)
        return false;
    return S_ISREG(st.st_mode) ? true : false;
}

// La fonction joinPath assemble deux morceaux de chemin (a et b) en un seul.
// - Si a est vide, elle retourne b directement.
// - Si b est vide, elle retourne a directement.
// - Si a se termine déjà par '/', elle concatène simplement a et b.
// - Sinon, elle insère un '/' entre a et b pour former un chemin valide.
// → Utile pour construire des chemins de fichiers de manière robuste
//   sans se soucier des séparateurs en double ou manquants.
std::string ConfigValidator::joinPath(const std::string &a, const std::string &b)
{
    if (a.empty())
        return b;
    if (b.empty())
        return a;
    if (a[a.size() - 1] == '/')
        return a + b;
    return a + "/" + b;
}

// La fonction stripLeadingSlash supprime un éventuel '/' au début d’un chemin.
// - Si la chaîne n’est pas vide et que son premier caractère est '/',
//   elle retourne la sous-chaîne à partir du deuxième caractère.
// - Sinon, elle retourne la chaîne telle quelle.
// → Utile pour normaliser des chemins en supprimant le slash initial.
std::string ConfigValidator::stripLeadingSlash(const std::string &p)
{
    if (!p.empty() && p[0] == '/')
        return p.substr(1);
    return p;
}

// La fonction normalizeRoot normalise le chemin racine d’une configuration,
// en tenant compte d’une base éventuelle.
// - Si base est vide, elle retourne root tel quel.
// - Si root commence par '/' → c’est déjà un chemin absolu, on retourne root directement.
// - Si root commence par "./" ou "../" → c’est un chemin relatif explicite,
//   on le retourne également tel quel sans le modifier.
// - Dans les autres cas, root est considéré comme relatif à base :
//   la fonction les assemble avec joinPath(base, root).
// → Cette fonction garantit que la racine est correctement interprétée,
//   qu’elle soit absolue, relative explicite ou relative à une base.
std::string ConfigValidator::normalizeRoot(const std::string &base, const std::string &root)
{
    if (base.empty())
        return root;
    if (!root.empty() && (root[0] == '/' || (root.size() > 1 && root[0] == '.' && (root[1] == '/' || (root.size()>2 && root[1]=='.' && root[2]=='/')))))
        return root;
    return joinPath(base, root);
}

// La fonction validate vérifie la validité d’une configuration complète (cfg).
// - Elle récupère la liste des blocs serveurs définis dans la configuration.
// - Si cette liste est vide, elle lève une ValidationError car il faut
//   au moins un serveur défini.
// - Sinon, elle parcourt chaque bloc serveur et appelle validateServer()
//   pour effectuer les vérifications nécessaires sur ce serveur, en passant
//   son index (i) et le répertoire de base (baseDir).
// → Cette fonction constitue le point d’entrée de la validation globale.
void ConfigValidator::validate(const Config &cfg, const std::string &baseDir)
{
    const std::vector<ServerBlock> &servers = cfg.getServers();
    if (servers.empty())
        throw ValidationError("No servers defined in configuration");

    for (size_t i = 0; i < servers.size(); ++i)
    {
        validateServer(servers[i], i, baseDir);
    }
}

// La fonction validateServer vérifie la validité d’un bloc serveur spécifique.
// - Elle récupère la liste des directives "listen" du serveur.
//   * Si aucune n’est définie, elle lève une ValidationError : un serveur doit écouter sur au moins un port.
// - Elle parcourt ensuite chaque "listen" pour vérifier la validité du port
//   (entre 1 et 65535). En cas d’erreur, une exception est levée.
// - Elle calcule la racine effective du serveur avec normalizeRoot(baseDir, srv.getRoot()).
//   * Si cette racine est vide, une ValidationError est levée.
//   * Si le chemin n’existe pas ou n’est pas un répertoire, une ValidationError est levée également.
// - Elle appelle ensuite validateErrorPages pour vérifier la validité des pages d’erreur configurées.
// - Enfin, elle appelle validateLocations pour analyser la validité des blocs location du serveur.
// → Cette fonction garantit qu’un bloc serveur est correctement défini et cohérent
//   avant que le serveur n’utilise la configuration.
void ConfigValidator::validateServer(const ServerBlock &srv, size_t serverIndex, const std::string &baseDir)
{
    const std::vector<ListenEntry> &ls = srv.getListens();
    if (ls.empty())
        throw ValidationError("Server #" + toStringSizeT(serverIndex) + ": no 'listen' directives");

    for (size_t j = 0; j < ls.size(); ++j)
    {
        if (!isValidPort(ls[j].port))
            throw ValidationError("Server #" + toStringSizeT(serverIndex) + ": invalid port " + toStringSizeT(ls[j].port));
    }

    std::string effectiveRoot = normalizeRoot(baseDir, srv.getRoot());
    if (effectiveRoot.empty())
        throw ValidationError("Server #" + toStringSizeT(serverIndex) + ": 'root' is empty");

    if (!pathExists(effectiveRoot) || !isDir(effectiveRoot))
        throw ValidationError("Server #" + toStringSizeT(serverIndex) + ": root directory not found or not a directory: " + effectiveRoot);

    validateErrorPages(srv, effectiveRoot, serverIndex);

    validateLocations(srv, effectiveRoot, serverIndex);
}

// La fonction validateErrorPages vérifie la validité des pages d’erreur
// configurées dans un bloc serveur.
// - Elle récupère la map des pages d’erreur (code HTTP → chemin associé).
// - Pour chaque entrée :
//     * code = code d’erreur HTTP (ex : 404, 500),
//     * rel  = chemin configuré pour cette page.
// - Elle construit le chemin absolu sous la racine effective :
//     * si rel commence par '/', on supprime ce slash et on l’ajoute après effectiveRoot,
//     * sinon, on le considère déjà comme relatif à effectiveRoot.
// - Elle vérifie ensuite que ce chemin existe ET correspond bien à un fichier régulier.
// - Si ce n’est pas le cas, elle lève une ValidationError avec un message
//   précisant le serveur concerné, le code d’erreur et le chemin invalide.
// → Cette fonction s’assure que toutes les pages d’erreur configurées sont présentes
//   et accessibles dans l’arborescence du serveur.
void ConfigValidator::validateErrorPages(const ServerBlock &srv, const std::string &effectiveRoot, size_t serverIndex)
{
    const std::map<int, std::string> &eps = srv.getErrorPages();
    for (std::map<int,std::string>::const_iterator it = eps.begin(); it != eps.end(); ++it)
    {
        int code = it->first;
        const std::string& rel = it->second;
        std::string underRoot;

        if (!rel.empty() && rel[0] == '/')
            underRoot = joinPath(effectiveRoot, stripLeadingSlash(rel));
        else
            underRoot = joinPath(effectiveRoot, rel);

        if (!pathExists(underRoot) || !isFile(underRoot))
        {
            std::ostringstream oss;
            oss << "Server #" << serverIndex << ": error_page " << code << " points to missing file: " << underRoot;
            throw ValidationError(oss.str());
        }
    }
}

// La fonction validateLocations vérifie la validité des blocs "location"
// définis dans un bloc serveur.
// - Elle récupère la liste des locations du serveur.
// - Pour chaque location :
//     * Elle calcule la racine effective :
//         - si loc.getRoot() est vide → on reprend la racine effective du serveur,
//         - sinon, on normalise loc.getRoot() par rapport à effectiveRoot.
//     * Elle vérifie que cette racine existe et correspond à un répertoire,
//       sinon, une ValidationError est levée avec un message précisant
//       le serveur, le chemin de la location et la racine invalide.
//     * Elle récupère ensuite l’upload_store de la location (up).
//         - si ce chemin n’est pas vide, il est normalisé par rapport à la racine,
//         - puis vérifié comme existant et étant un répertoire.
//         - en cas d’échec, une ValidationError est levée avec un message détaillé.
// → Cette fonction garantit que chaque location est correctement configurée,
//   avec une racine et un éventuel répertoire d’upload valides.
void ConfigValidator::validateLocations(const ServerBlock &srv, const std::string &effectiveRoot, size_t serverIndex)
{
    const std::vector<LocationBlock> &locs = srv.getLocations();
    for (size_t k = 0; k < locs.size(); ++k)
    {
        const LocationBlock& loc = locs[k];

        std::string lroot = loc.getRoot().empty() ? effectiveRoot : normalizeRoot(effectiveRoot, loc.getRoot());
        if (!pathExists(lroot) || !isDir(lroot))
        {
            std::ostringstream oss;
            oss << "Server #" << serverIndex << " location '" << loc.getPathPrefix() << "': root not found or not a directory: " << lroot;
            throw ValidationError(oss.str());
        }

        const std::string &up = loc.getUploadStore();
        if (!up.empty())
        {
            std::string upAbs = normalizeRoot(lroot, up);
            if (!pathExists(upAbs) || !isDir(upAbs))
            {
                std::ostringstream oss;
                oss << "Server #" << serverIndex << " location '" << loc.getPathPrefix() << "': upload_store directory not found or not a directory: " << upAbs;
                throw ValidationError(oss.str());
            }
        }
    }
}
