#include "utils/DirectoryResolver.hpp"
#include "utils/AutoIndex.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <cerrno>
#include <cstring>

// La fonction joinPath assemble deux morceaux de chemin (a et b) en un seul.
// - Si a est vide, elle retourne b directement.
// - Si a se termine déjà par '/', elle concatène simplement a et b.
// - Sinon, elle insère un '/' entre a et b.
// → Cette version simplifiée ne gère pas le cas où b est vide
//   (contrairement à d’autres variantes), mais elle suffit
//   pour construire un chemin correct à partir de deux segments.
static std::string joinPath(const std::string &a, const std::string &b)
{
    if (a.empty())
        return b;
    if (a[a.size() - 1] == '/')
        return a + b;
    return a + "/" + b;
}

// La fonction isDirectory vérifie si un chemin absolu correspond à un répertoire.
// - Elle déclare une structure stat pour stocker les informations sur le chemin.
// - Elle appelle la fonction système stat() avec absPath.
//   * Si stat échoue (valeur de retour différente de 0), elle retourne false.
// - Sinon, elle utilise la macro S_ISDIR(st.st_mode) pour tester si le chemin
//   correspond à un répertoire.
//   * Retourne true si c’est un répertoire, false sinon.
// → Utile pour valider qu’un chemin donné pointe bien vers un dossier existant.
bool isDirectory(const std::string &absPath)
{
    struct stat st;
    if (::stat(absPath.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode) ? true : false;
}

// La fonction fileExists vérifie si un chemin absolu correspond à un fichier régulier.
// - Elle déclare une structure stat pour stocker les informations sur le chemin.
// - Elle appelle la fonction système stat() avec absPath.
//   * Si stat échoue (valeur de retour différente de 0), elle retourne false.
// - Sinon, elle utilise la macro S_ISREG(st.st_mode) pour tester si le chemin
//   correspond à un fichier régulier (et non un dossier, lien, etc.).
//   * Retourne true si c’est un fichier, false sinon.
// → Utile pour confirmer qu’un fichier existe avant de l’utiliser.
bool fileExists(const std::string &absPath)
{
    struct stat st;
    if (::stat(absPath.c_str(), &st) != 0)
        return false;
    return S_ISREG(st.st_mode) ? true : false;
}

// La fonction findIndexFile cherche un fichier d’index dans un répertoire donné.
// - Elle parcourt la liste indexList qui contient des noms de fichiers d’index possibles
//   (ex : "index.html", "index.htm").
// - Pour chaque nom, elle construit le chemin absolu correspondant en l’assemblant
//   avec dirAbsPath via joinPath().
// - Elle vérifie ensuite si ce fichier existe avec fileExists().
//   * Si oui :
//       - elle affecte ce chemin à outPath,
//       - retourne true pour signaler qu’un fichier d’index a été trouvé.
// - Si aucun fichier de la liste n’est présent dans le répertoire, la fonction retourne false.
// → Cette fonction est utile pour localiser automatiquement la page d’index
//   à renvoyer lorsqu’un client demande un répertoire.
bool findIndexFile(const std::string &dirAbsPath, const std::vector<std::string> &indexList, std::string &outPath)
{
    for (std::vector<std::string>::const_iterator it = indexList.begin(); it != indexList.end(); ++it)
    {
        std::string p = joinPath(dirAbsPath, *it);
        if (fileExists(p))
        {
            outPath = p;
            return true;
        }
    }
    return false;
}

// La fonction resolveDirectoryRequest détermine comment répondre à une requête
// visant un répertoire (dirAbsPath).
// - Elle initialise un résultat (DirResolveResult r) avec la décision Forbidden.
// - Étape 1 : chercher un fichier d’index.
//     * Si findIndexFile trouve un fichier parmi indexList,
//       la décision devient ServeIndexFile,
//       le chemin du fichier est enregistré dans r.file_path,
//       et la fonction retourne ce résultat.
// - Étape 2 : si aucun index n’a été trouvé mais que autoindexOn est activé,
//       la décision devient ServeAutoIndex,
//       la page HTML de listing est générée via generateAutoIndex(),
//       puis la fonction retourne ce résultat.
// - Étape 3 : si ni fichier d’index ni autoindex ne sont disponibles,
//       la décision reste Forbidden, et la fonction retourne ce résultat.
// → Cette fonction applique donc la logique habituelle des serveurs web
//   pour répondre à une requête sur un répertoire.
DirResolveResult resolveDirectoryRequest(const std::string &dirAbsPath, const std::string &urlPath, const std::vector<std::string> &indexList, bool autoindexOn)
{
    DirResolveResult r;
    r.decision = Forbidden;

    // 1) Try index files
    std::string idx;
    if (findIndexFile(dirAbsPath, indexList, idx))
    {
        r.decision = ServeIndexFile;
        r.file_path = idx;
        return r;
    }

    // 2) Autoindex
    if (autoindexOn)
    {
        r.decision = ServeAutoIndex;
        r.html = generateAutoIndex(dirAbsPath, urlPath);
        return r;
    }

    // 3) Otherwise, forbidden
    r.decision = Forbidden;
    return r;
}
