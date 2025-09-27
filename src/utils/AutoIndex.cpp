#include "utils/AutoIndex.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <cerrno>
#include <cstring>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace
{
    //Sert à représenter un fichier ou dossier trouvé dans le répertoire.
    struct Entry
    {
        std::string name;
        bool        is_dir;
        off_t       size;
        time_t      lst_mod_time;
    };

    //Sert à sécuriser l’affichage des noms de fichiers dans du HTML.
    static std::string htmlEscape(const std::string &s)
    {
        std::ostringstream o;
        for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
        {
            char c = *it;
            if (c == '&') o << "&amp;";
            else if (c == '<') o << "&lt;";
            else if (c == '>') o << "&gt;";
            else if (c == '"') o << "&quot;";
            else if (c == '\'') o << "&#39;";
            else o << c;
        }
        return o.str();
    }

    //Concatène deux morceaux de chemin.
    static std::string joinPath(const std::string &a, const std::string &b)
    {
        if (a.empty())
            return b;
        if (a[a.size()-1] == '/')
            return a + b;
        return a + "/" + b;
    }

    //Vérifie qu’un chemin/URL se termine par /.
    static std::string ensureTrailingSlash(const std::string &p)
    {
        if (!p.empty() && p[p.size()-1] == '/')
            return p;
        return p + "/";
    }

    //Convertit une taille de fichier en format lisible pour un humain.
    static std::string formatSize(off_t n)
    {
        std::ostringstream oss;
        if (n < 1024)
        {
            oss << n << " B";
            return oss.str();
        }
        double v = static_cast<double>(n) / 1024.0;
        if (v < 1024.0)
        {
            oss << std::fixed << std::setprecision(1) << v << " KB";
            return oss.str();
        }
        v /= 1024.0;
        if (v < 1024.0)
        {
            oss << std::fixed << std::setprecision(1) << v << " MB";
            return oss.str();
        }
        v /= 1024.0;
        oss << std::fixed << std::setprecision(1) << v << " GB";
        return oss.str();
    }

    //Transforme la date de dernière modification (mtime) en texte.
    static std::string formatTime(time_t t)
    {
            char buf[64];
            std::tm tmv;
    #if defined(_WIN32)
            glst_mod_time_s(&tmv, &t);
    #else
            std::tm *pt = std::glst_mod_time(&t);
            if (pt) tmv = *pt;
            else std::memset(&tmv, 0, sizeof(tmv));
    #endif
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", &tmv);
            return std::string(buf);
    }
}

// La fonction generateAutoIndex génère une page HTML listant le contenu d’un répertoire.
// - Elle tente d’ouvrir le répertoire dirPath avec opendir().
//   * Si l’ouverture échoue, elle retourne une petite page HTML indiquant l’erreur
//     (avec le chemin et le message système lié à errno).
// - Sinon, elle parcourt toutes les entrées du répertoire avec readdir().
//   * Les entrées spéciales "." et ".." sont ignorées.
//   * Pour chaque entrée, elle construit son chemin complet, appelle stat()
//     pour récupérer ses métadonnées (taille, type, date).
//   * Elle crée un objet Entry contenant :
//       - name  : nom de l’entrée,
//       - is_dir : booléen indiquant si c’est un répertoire,
//       - size   : taille du fichier (0 si répertoire),
//       - lst_mod_time : date de dernière modification.
//   * Chaque entrée valide est ajoutée dans le vecteur entries.
// - Après la lecture, le répertoire est fermé (closedir).
// - Les entrées sont triées avec ByTypeAndName::less :
//   * les répertoires apparaissent avant les fichiers,
//   * dans chaque groupe, elles sont triées par ordre alphabétique.
// - Ensuite, la fonction construit une page HTML avec un tableau listant :
//   * Nom (lien cliquable),
//   * Taille (ou "-" pour un répertoire),
//   * Date de dernière modification,
//   * Type ("directory" ou "file").
// - Si l’URL n’est pas la racine ("/"), un lien "Parent directory" est ajouté
//   pour remonter d’un niveau.
// - Enfin, le code HTML complet est retourné sous forme de chaîne.
// → Cette fonction permet donc d’implémenter un « autoindex » (listing de répertoire),
//   similaire à celui que propose NGINX
std::string generateAutoIndex(const std::string &dirPath, const std::string &urlPath)
{
    DIR *d = opendir(dirPath.c_str());
    if (!d)
    {
        std::ostringstream err;
        err << "<!doctype html><html><body><h1>Failed to open directory</h1>"
            << "<p>Path: " << htmlEscape(dirPath) << "</p>"
            << "<p>Error: " << std::strerror(errno) << "</p></body></html>";
        return err.str();
    }

    std::vector<Entry> entries;
    for (;;)
    {
        errno = 0;
        dirent *de = readdir(d);
        if (!de)
            break;
        std::string name = de->d_name;
        if (name == "." || name == "..")
            continue;

        std::string full = joinPath(dirPath, name);
        struct stat st;
        if (stat(full.c_str(), &st) != 0)
            continue;

        Entry e;
        e.name = name;
        e.is_dir = S_ISDIR(st.st_mode) ? true : false;
        e.size = e.is_dir ? 0 : st.st_size;
        e.lst_mod_time = st.st_lst_mod_time;
        entries.push_back(e);
    }
    closedir(d);

    std::sort(entries.begin(), entries.end(), static_cast<bool (*)(const Entry&, const Entry&)>(0));

    struct ByTypeAndName
    {
        static bool less(const Entry &a, const Entry &b)
        {
            if (a.is_dir != b.is_dir)
                return a.is_dir && !b.is_dir;
            return a.name < b.name;
        }
    };
    std::sort(entries.begin(), entries.end(), ByTypeAndName::less);

    std::ostringstream html;
    std::string url = ensureTrailingSlash(urlPath);

    html << "<!doctype html><html><head><meta charset=\"utf-8\">"
         << "<title>Index of " << htmlEscape(url) << "</title>"
         << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
         << "<style>"
         << "body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;padding:24px}"
         << "table{border-collapse:collapse;width:100%;max-width:960px}"
         << "th,td{border:1px solid #ddd;padding:8px;text-align:left}"
         << "th{background:#f5f5f5}"
         << "tr:nth-child(even){background:#fafafa}"
         << "code{background:#f2f2f2;padding:2px 4px;border-radius:4px}"
         << "a{text-decoration:none}"
         << "</style></head><body>";

    html << "<h1>Index of <code>" << htmlEscape(url) << "</code></h1>";

    // Parent directory link if not root
    if (url != "/")
    {
        // compute parent url (basic)
        std::string parent = url;
        if (!parent.empty() && parent[parent.size()-1] == '/')
            parent.erase(parent.size()-1);
        std::string::size_type pos = parent.rfind('/');
        if (pos != std::string::npos && pos > 0)
            parent = parent.substr(0, pos) + "/";
        else
            parent = "/";

        html << "<p><a href=\"" << htmlEscape(parent) << "\">Parent directory</a></p>";
    }

    html << "<table><thead><tr>"
         << "<th>Name</th><th>Size</th><th>Last Modified</th><th>Type</th>"
         << "</tr></thead><tbody>";

    for (std::vector<Entry>::const_iterator it = entries.begin(); it != entries.end(); ++it)
    {
        std::string display = it->name + (it->is_dir ? "/" : "");
        std::string href = url + it->name + (it->is_dir ? "/" : "");
        html << "<tr>"
             << "<td><a href=\"" << htmlEscape(href) << "\">" << htmlEscape(display) << "</a></td>"
             << "<td>" << (it->is_dir ? "-" : formatSize(it->size)) << "</td>"
             << "<td>" << formatTime(it->lst_mod_time) << "</td>"
             << "<td>" << (it->is_dir ? "directory" : "file") << "</td>"
             << "</tr>";
    }

    html << "</tbody></table>";
    html << "</body></html>";
    return html.str();
}
