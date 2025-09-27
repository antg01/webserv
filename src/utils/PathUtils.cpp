#include "utils/PathUtils.hpp"
#include <sstream>
#include <stdexcept>

// La fonction splitPath découpe une chaîne en plusieurs sous-chaînes
// selon un délimiteur donné (delim).
// - Elle crée un flux de lecture (istringstream) à partir de path.
// - Elle lit la chaîne morceau par morceau avec std::getline,
//   en utilisant delim comme séparateur.
// - Chaque morceau extrait est ajouté au vecteur parts.
// - Une fois le parcours terminé, elle retourne le vecteur de sous-chaînes.
// → Exemple : splitPath("/usr/local/bin", '/') renvoie {"", "usr", "local", "bin"}.
static std::vector<std::string> splitPath(const std::string &path, char delim)
{
    std::vector<std::string> parts;
    std::string token;
    std::istringstream ss(path);
    while (std::getline(ss, token, delim))
    {
        parts.push_back(token);
    }
    return parts;
}

// La fonction resolvePath construit un chemin absolu sécurisé à partir
// d’une racine (root) et d’un chemin de requête (requestPath).
// - Si root est vide, elle lève une exception (erreur de configuration).
// - Elle copie root dans cleanRoot et supprime l’éventuel '/' final
//   pour éviter les doublons.
// - Elle découpe requestPath en segments (séparés par '/') avec splitPath.
// - Pour chaque segment :
//     * si le segment est vide ou équivaut à ".", il est ignoré,
//     * si le segment est "..", on remonte d’un niveau en supprimant
//       le dernier élément de stack (s’il existe),
//     * sinon, le segment est ajouté à stack (construction du chemin normalisé).
// - Ensuite, elle reconstruit le chemin final avec un flux oss :
//     * on commence par cleanRoot,
//     * puis on ajoute chaque segment de stack précédé de '/'.
// - Le chemin final normalisé est retourné.
// → Exemple :
//     root = "/var/www", requestPath = "/images/../css/."
//     ⇒ "/var/www/css"
std::string resolvePath(const std::string &root, const std::string &requestPath)
{
    if (root.empty())
        throw std::runtime_error("resolvePath: root is empty");

    std::vector<std::string> stack;

    std::string cleanRoot = root;
    if (!cleanRoot.empty() && cleanRoot[cleanRoot.size()-1] == '/')
        cleanRoot.erase(cleanRoot.size()-1);

    std::vector<std::string> parts = splitPath(requestPath, '/');
    for (size_t i = 0; i < parts.size(); ++i)
    {
        const std::string& seg = parts[i];
        if (seg.empty() || seg == ".")
        {
            continue;
        }
        else if (seg == "..")
        {
            if (!stack.empty())
                stack.pop_back();
        }
        else
        {
            stack.push_back(seg);
        }
    }

    std::ostringstream oss;
    oss << cleanRoot;
    for (size_t i = 0; i < stack.size(); ++i)
    {
        oss << "/" << stack[i];
    }

    return oss.str();
}
