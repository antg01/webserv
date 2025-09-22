#include "../../include/config/ConfigParser.hpp"
#include <stdexcept>
#include <cstdlib>

// ------------------ Lexer ------------------
// La fonction load charge le contenu d’un fichier texte dans le lexer.
// - Elle ouvre le fichier spécifié par "path" en lecture.
// - Si le fichier ne peut pas être ouvert, une exception std::runtime_error est levée.
// - Elle lit le fichier ligne par ligne, et stocke tout le contenu dans un flux mémoire (ostringstream).
// - Chaque ligne lue est ajoutée suivie d’un retour à la ligne ("\n").
// - Une fois le fichier entièrement lu, le contenu est converti en chaîne de caractères
//   et stocké dans l’attribut src_.
// - Les attributs pos_ (position courante dans le texte) et line_ (numéro de ligne courante)
//   sont réinitialisés respectivement à 0 et 1 pour commencer une nouvelle analyse lexicale.
void Lexer::load(const std::string &path)
{
    std::ifstream in(path.c_str());
    if (!in)
        throw std::runtime_error("Cannot open config file: " + path);

    std::ostringstream buf;
    std::string line;
    line.reserve(1024);
    while (std::getline(in, line))
    {
        buf << line << "\n";
    }
    src_ = buf.str();
    pos_ = 0;
    line_ = 1;
}

// La fonction eof vérifie si la lecture du texte est terminée.
// - Elle compare la position courante (pos_) à la taille totale de la source (src_.size()).
// - Si pos_ est supérieur ou égal à la taille du texte, cela signifie que l’on est arrivé
//   à la fin du fichier (End Of File).
// - La fonction retourne true si la fin est atteinte, false sinon.
bool Lexer::eof() const
{
    return pos_ >= src_.size();
}

// La fonction peek permet de regarder le caractère courant dans la source sans avancer la position.
// - Elle vérifie d’abord si la fin du texte est atteinte en appelant eof().
// - Si on est arrivé à la fin, elle retourne le caractère nul ('\0') pour signaler qu’il n’y a plus rien à lire.
// - Sinon, elle retourne le caractère situé à la position actuelle (pos_) dans la chaîne source (src_).
char Lexer::peek() const
{
    if (eof())
        return '\0';
    return src_[pos_];
}

// La fonction get lit et renvoie le caractère courant de la source, puis avance la position.
// - Elle commence par vérifier si la fin du texte est atteinte avec eof().
//   Si c’est le cas, elle retourne le caractère nul ('\0') pour signaler qu’il n’y a plus rien à lire.
// - Sinon, elle lit le caractère situé à la position actuelle (pos_), puis incrémente pos_ pour avancer d’un cran.
// - Si le caractère lu est un retour à la ligne ('\n'), elle incrémente le compteur de ligne (line_).
// - Enfin, elle retourne le caractère lu.
char Lexer::get()
{
    if (eof())
        return '\0';
    char c = src_[pos_++];
    if (c == '\n') line_ += 1;
    return c;
}

// La fonction skipSpacesAndComments avance la lecture en ignorant les espaces et les commentaires.
// - Elle boucle indéfiniment jusqu’à ce qu’il n’y ait plus ni espaces, ni commentaires à sauter.
// - Dans un premier temps, elle consomme tous les caractères d’espacement (espaces, tabulations, retours à la ligne, etc.)
//   en appelant get() tant que peek() renvoie un caractère blanc.
// - Ensuite, si le caractère courant est un '#', cela signifie qu’il s’agit d’un commentaire.
//   Dans ce cas, elle lit et ignore tous les caractères jusqu’à la fin de la ligne (caractère '\n').
//   Une fois le commentaire ignoré, elle recommence la boucle pour vérifier si d’autres espaces ou commentaires suivent.
// - Si aucun espace ni commentaire n’est trouvé, la boucle se termine.
// → Cette fonction permet donc de se positionner directement sur le prochain caractère significatif du code source.
void Lexer::skipSpacesAndComments()
{
    for (;;)
    {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek())))
        {
            get();
        }
        if (!eof() && peek() == '#')
        {
            while (!eof() && get() != '\n') {}
            continue;
        }
        break;
    }
}

// La fonction isIdentChar vérifie si un caractère est autorisé dans un identifiant, un chemin ou un hôte.
// - Elle accepte toutes les lettres et chiffres (std::isalnum).
// - Elle accepte aussi les caractères spéciaux suivants :
//   '_' (underscore), '.' (point), '-' (tiret), '/' (slash), ':' (deux-points).
// - Elle tolère également '[' et ']' afin de supporter des adresses IPv6 simples comme [::].
// - Si le caractère appartient à cet ensemble, la fonction retourne true ; sinon false.
static inline bool isIdentChar(char c)
{
    if (std::isalnum(static_cast<unsigned char>(c)))
        return true;
    if (c == '_' || c == '.' || c == '-' || c == '/' || c == ':' || c == '[' || c == ']')
        return true;
    return false;
}

// La fonction readIdent lit un identifiant à partir de la position courante dans la source.
// - Elle mémorise d’abord la position de départ (start).
// - Ensuite, elle parcourt la source caractère par caractère tant que :
//     * la fin du texte n’est pas atteinte (eof() == false),
//     * le caractère courant est valide pour un identifiant (isIdentChar),
//     * et que ce caractère n’est pas un délimiteur : ';', '{', '}', '"', ou '#' (début de commentaire).
// - La boucle s’arrête dès qu’un caractère interdit ou un délimiteur est rencontré.
// - Enfin, la fonction construit et retourne un objet Token de type T_IDENT,
//   contenant la sous-chaîne correspondant à l’identifiant lu ainsi que le numéro de ligne courant.
Token Lexer::readIdent()
{
    size_t start = pos_;
    while (!eof())
    {
        char c = peek();
        if (!isIdentChar(c) || c == ';' || c == '{' || c == '}' || c == '"' || c == '#')
            break;
        get();
    }
    return Token(Token::T_IDENT, src_.substr(start, pos_ - start), line_);
}

// La fonction readNumberOrIdent lit une séquence de caractères à partir de la position courante
// et détermine si elle représente un nombre ou un identifiant.
// - Elle mémorise la position de départ (start) et initialise un indicateur allDigits à true.
// - Ensuite, elle parcourt la source tant que :
//     * la fin n’est pas atteinte,
//     * le caractère courant est valide pour un identifiant (isIdentChar),
//     * et que ce caractère n’est pas un délimiteur (';', '{', '}', '"', '#').
// - Pendant la lecture, si un caractère n’est pas un chiffre, allDigits passe à false.
// - Une fois la séquence lue, elle extrait la sous-chaîne correspondante.
// - Si cette sous-chaîne est vide, elle retourne un token T_IDENT vide (sécurité).
// - Sinon :
//     * si tous les caractères étaient des chiffres → retourne un token T_NUMBER,
//     * sinon → retourne un token T_IDENT.
Token Lexer::readNumberOrIdent()
{
    size_t start = pos_;
    bool allDigits = true;

    while (!eof())
    {
        char c = peek();
        if (!isIdentChar(c) || c == ';' || c == '{' || c == '}' || c == '"' || c == '#')
            break;
        if (!std::isdigit(static_cast<unsigned char>(c)))
            allDigits = false;
        get();
    }

    std::string s = src_.substr(start, pos_ - start);
    if (s.empty())
        return Token(Token::T_IDENT, s, line_); // fallback

    if (allDigits)
        return Token(Token::T_NUMBER, s, line_);
    return Token(Token::T_IDENT, s, line_);
}

// La fonction readString lit une chaîne de caractères délimitée par des guillemets.
// - Elle suppose que le caractère courant est déjà un guillemet ouvrant (").
// - Elle consomme ce guillemet puis mémorise la position de départ du contenu de la chaîne.
// - Ensuite, elle lit caractère par caractère jusqu’à rencontrer un guillemet fermant ou la fin du texte :
//     * si le caractère lu est un antislash ('\') et qu’il reste des caractères,
//       elle consomme le caractère suivant pour ignorer la séquence d’échappement.
// - Si la fin du texte est atteinte avant de trouver un guillemet fermant,
//   la fonction retourne un token T_EOF avec une valeur vide (erreur de syntaxe).
// - Sinon, elle extrait la sous-chaîne entre les guillemets, consomme le guillemet fermant,
//   et retourne un token T_STRING contenant la valeur lue.
Token Lexer::readString()
{
    get();
    size_t start = pos_;
    while (!eof() && peek() != '"')
    {
        char c = get();
        if (c == '\\' && !eof())
        {
            get();
        }
    }
    if (eof())
        return Token(Token::T_EOF, "", line_);
    std::string s = src_.substr(start, pos_ - start);
    get();
    return Token(Token::T_STRING, s, line_);
}

// La fonction next lit et retourne le prochain token significatif du flux source.
// - Elle commence par ignorer les espaces et les commentaires (skipSpacesAndComments).
// - Si la fin du texte est atteinte, elle retourne un token de type T_EOF.
// - Sinon, elle regarde le caractère courant (peek) pour décider quel type de token produire :
//     * '{' → retourne un token T_LBRACE et consomme le caractère.
//     * '}' → retourne un token T_RBRACE et consomme le caractère.
//     * ';' → retourne un token T_SEMI et consomme le caractère.
//     * '"' → appelle readString() pour lire une chaîne de caractères.
//     * chiffre → appelle readNumberOrIdent() pour distinguer un nombre d’un identifiant.
//     * tout autre cas → appelle readIdent() pour lire un identifiant générique.
// → Cette fonction est le cœur du lexer : elle fournit le prochain élément lexical
//   que l’analyseur pourra ensuite traiter.
Token Lexer::next()
{
    skipSpacesAndComments();
    if (eof()) return Token(Token::T_EOF, "", line_);

    char c = peek();

    if (c == '{')
    {
        get();
        return Token(Token::T_LBRACE, "{", line_);
    }
    if (c == '}')
    {
        get();
        return Token(Token::T_RBRACE, "}", line_);
    }
    if (c == ';')
    {
        get();
        return Token(Token::T_SEMI, ";", line_);
    }
    if (c == '"')
    {
        return readString();
    }

    if (std::isdigit(static_cast<unsigned char>(c)))
        return readNumberOrIdent();

    return readIdent();
}

// ------------------ Parser ------------------
// Le constructeur par défaut de ConfigParser initialise ses membres :
// - lex_ est construit avec son constructeur par défaut
// - cur_ est également construit avec son constructeur par défaut (le token courant).
ConfigParser::ConfigParser() : lex_(), cur_() {}

// La fonction advance fait progresser l’analyse en lisant le prochain token.
// - Elle appelle lex_.next() pour obtenir le prochain élément lexical depuis le lexer.
// - Elle stocke ce token dans cur_, qui représente le token courant du parseur.
// → Cette fonction permet donc au parseur d’avancer dans le flux de tokens.
void ConfigParser::advance()
{
    cur_ = lex_.next();
}

// La fonction accept vérifie si le token courant est du type attendu (t).
// - Si le type du token courant (cur_.type) correspond à t :
//     * elle appelle advance() pour consommer ce token et passer au suivant,
//     * puis retourne true pour signaler que le token a bien été reconnu.
// - Sinon, elle ne change rien et retourne false.
// → Cette fonction est utilisée dans le parseur pour tester la présence optionnelle
//   d’un token sans générer d’erreur si celui-ci est absent.
bool ConfigParser::accept(Token::Type t)
{
    if (cur_.type == t)
    {
        advance();
        return true;
    }
    return false;
}

// La fonction expect vérifie que le token courant est bien du type attendu (t).
// - Elle utilise accept(t) pour tenter de consommer ce token.
// - Si accept échoue (c’est-à-dire que le token n’est pas du bon type),
//   elle lève une exception ParseError avec un message indiquant ce qui était attendu,
//   et précise également la ligne où l’erreur s’est produite (cur_.line).
// → Cette fonction sert à imposer la présence obligatoire d’un token particulier
//   dans la grammaire analysée.
void ConfigParser::expect(Token::Type t, const char *what)
{
    if (!accept(t))
        throw ParseError(std::string("Expected ") + what, cur_.line);
}

// La fonction parseFile analyse un fichier de configuration complet.
// - Elle commence par charger le contenu du fichier indiqué par path dans le lexer (lex_.load).
// - Ensuite, elle appelle advance() pour initialiser le token courant (cur_) avec le premier token.
// - Enfin, elle lance l’analyse complète du fichier en appelant parseConfig(),
//   et retourne l’objet Config produit par cette analyse.
Config ConfigParser::parseFile(const std::string &path)
{
    lex_.load(path);
    advance();
    return parseConfig();
}

// La fonction parseConfig analyse la structure générale d’un fichier de configuration.
// - Elle crée un objet Config vide (cfg) qui contiendra le résultat de l’analyse.
// - Tant que le token courant n’est pas de type T_EOF (fin de fichier) :
//     * Si le token courant est un identifiant avec le texte "server",
//       alors elle appelle parseServer() pour analyser un bloc serveur,
//       puis ajoute ce bloc au Config via cfg.addServer().
//     * Sinon, si le token courant n’est pas "server", une exception ParseError est levée
//       pour signaler une erreur de syntaxe (un bloc attendu n’a pas été trouvé).
// - Quand la boucle se termine (fin du fichier atteinte), la fonction retourne
//   l’objet Config complété.
// → En résumé, cette fonction construit la configuration en ne reconnaissant que des blocs "server".
Config ConfigParser::parseConfig()
{
    Config cfg;
    while (cur_.type != Token::T_EOF)
    {
        if (cur_.type == Token::T_IDENT && cur_.text == "server")
        {
            ServerBlock srv = parseServer();
            cfg.addServer(srv);
        }
        else
        {
            throw ParseError("Expected 'server' block", cur_.line);
        }
    }
    return cfg;
}

// La fonction parseServer analyse un bloc "server" du fichier de configuration.
// - Elle crée un objet ServerBlock vide (srv) qui va contenir les directives du serveur.
// - Elle appelle advance() pour consommer le mot-clé "server" déjà rencontré.
// - Elle vérifie ensuite que le prochain token est bien une accolade ouvrante '{'
//   grâce à expect(Token::T_LBRACE, "'{'").
// - Puis elle entre dans une boucle qui se poursuit jusqu’à trouver une accolade fermante '}' :
//     * Si la fin du fichier est atteinte avant la fermeture du bloc,
//       une exception ParseError est levée (bloc "server" non refermé).
//     * Sinon, elle appelle parseServerDirective(srv) pour analyser et stocker
//       une directive à l’intérieur du bloc serveur.
// - Une fois la boucle terminée, elle vérifie la présence obligatoire de l’accolade fermante '}'.
// - Enfin, elle retourne l’objet ServerBlock construit.
// → Cette fonction assure donc la lecture et la validation d’un bloc "server" complet.
ServerBlock ConfigParser::parseServer()
{
    ServerBlock srv;
    advance();
    expect(Token::T_LBRACE, "'{'");
    while (cur_.type != Token::T_RBRACE)
    {
        if (cur_.type == Token::T_EOF)
            throw ParseError("Unclosed server block", cur_.line);
        parseServerDirective(srv);
    }
    expect(Token::T_RBRACE, "'}'");
    return srv;
}

// La fonction parseServerDirective analyse une directive à l’intérieur d’un bloc "server".
// - Elle commence par vérifier que le token courant est bien un identifiant (nom de directive).
//   Sinon, elle lève une erreur "Expected directive name".
// - Ensuite, selon le texte du token courant (cur_.text), elle appelle la fonction spécialisée
//   qui gère la directive correspondante :
//     * "listen"                → dir_listen(srv)
//     * "root"                  → dir_root(srv)
//     * "index"                 → dir_index(srv)
//     * "server_name"           → dir_server_name(srv)
//     * "client_max_body_size"  → dir_client_max_body_size(srv)
//     * "error_page"            → dir_error_page(srv)
//     * "location"              → dir_location(srv)
// - Chaque fonction appelée lit les arguments nécessaires et modifie srv en conséquence.
// - Si la directive n’est pas reconnue, la fonction lève une erreur ParseError
//   avec un message "Unknown server directive" et le nom rencontré.
// → Cette fonction agit comme un "dispatch" qui redirige vers le bon analyseur
//   en fonction du mot-clé rencontré.
void ConfigParser::parseServerDirective(ServerBlock &srv)
{
    if (cur_.type != Token::T_IDENT)
        throw ParseError("Expected directive name", cur_.line);

    if (cur_.text == "listen")
    {
        dir_listen(srv);
        return;
    }
    if (cur_.text == "root")
    {
        dir_root(srv);
        return;
    }
    if (cur_.text == "index")
    {
        dir_index(srv);
        return;
    }
    if (cur_.text == "server_name")
    {
        dir_server_name(srv);
        return;
    }
    if (cur_.text == "client_max_body_size")
    {
        dir_client_max_body_size(srv);
        return;
    }
    if (cur_.text == "error_page")
    {
        dir_error_page(srv);
        return;
    }
    if (cur_.text == "location")
    {
        dir_location(srv);
        return;
    }

    throw ParseError("Unknown server directive: " + cur_.text, cur_.line);
}

// La fonction dir_listen analyse et enregistre une directive "listen" dans un bloc serveur.
// - Elle commence par consommer le mot-clé "listen" (advance).
// - Puis elle vérifie que le token suivant est bien une chaîne ou un identifiant,
//   correspondant à une valeur de type "host:port" ou simplement "port".
//   Si ce n’est pas le cas, une erreur ParseError est levée.
// - Elle mémorise ensuite ce texte (cur_.text), avance d’un token, puis vérifie
//   la présence obligatoire d’un point-virgule final (';') avec expect.
// - Ensuite, elle initialise deux variables (host et port) et appelle splitHostPort()
//   pour découper la valeur fournie en adresse et port numériques.
// - Enfin, elle enregistre ces informations dans le bloc serveur via srv.addListen(host, port).
// → Cette fonction permet donc de configurer sur quelle adresse et port
//   le serveur doit écouter.
void ConfigParser::dir_listen(ServerBlock &srv)
{
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("listen expects 'host:port' or 'port'", cur_.line);
    std::string s = cur_.text;
    advance();
    expect(Token::T_SEMI, "';'");
    std::string host;
    int port = 0;
    splitHostPort(s, host, port);
    srv.addListen(host, port);
}

// La fonction dir_root analyse une directive "root" dans un bloc serveur.
// - Elle consomme d’abord le mot-clé "root" (advance).
// - Puis elle vérifie que le token suivant est bien un identifiant ou une chaîne,
//   représentant le chemin du répertoire racine à définir.
//   Si ce n’est pas le cas, elle lève une ParseError avec le message "root expects a path".
// - Si le chemin est valide, elle l’enregistre dans le bloc serveur via srv.setRoot(cur_.text).
// - Ensuite, elle avance pour consommer ce token de chemin.
// - Enfin, elle vérifie la présence obligatoire d’un point-virgule final (';') avec expect.
// → Cette fonction permet donc de configurer le répertoire racine (root directory) du serveur.
void ConfigParser::dir_root(ServerBlock &srv)
{
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("root expects a path", cur_.line);
    srv.setRoot(cur_.text);
    advance();
    expect(Token::T_SEMI, "';'");
}

// La fonction dir_index analyse une directive "index" dans un bloc serveur.
// - Elle consomme d’abord le mot-clé "index" (advance).
// - Elle vérifie ensuite que le token suivant est bien un identifiant ou une chaîne,
//   ce qui correspond à un ou plusieurs noms de fichiers d’index attendus (ex: "index.html").
//   Si ce n’est pas le cas, elle lève une ParseError avec le message
//   "index expects at least one filename".
// - Tant que le token courant est un identifiant ou une chaîne, elle ajoute ce nom
//   à la configuration du serveur via srv.addIndex(cur_.text), puis avance.
// - Quand il n’y a plus de noms d’index, elle vérifie la présence obligatoire d’un
//   point-virgule final (';') avec expect.
// → Cette fonction permet donc de définir la ou les pages d’index par défaut
//   utilisées par le serveur.
void ConfigParser::dir_index(ServerBlock &srv)
{
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("index expects at least one filename", cur_.line);
    while (cur_.type == Token::T_IDENT || cur_.type == Token::T_STRING)
    {
        srv.addIndex(cur_.text);
        advance();
    }
    expect(Token::T_SEMI, "';'");
}

// La fonction dir_server_name analyse une directive "server_name" dans un bloc serveur.
// - Elle consomme d’abord le mot-clé "server_name" (advance).
// - Elle vérifie que le token suivant est bien un identifiant ou une chaîne,
//   représentant le nom du serveur à définir (ex: "example.com").
//   Si ce n’est pas le cas, elle lève une ParseError avec le message
//   "server_name expects a token".
// - Si le token est valide, elle l’enregistre dans le bloc serveur via srv.setServerName(cur_.text).
// - Ensuite, elle avance pour consommer ce token.
// - Enfin, elle vérifie la présence obligatoire d’un point-virgule final (';') avec expect.
// → Cette fonction permet donc d’associer un nom de domaine ou un alias au serveur.
void ConfigParser::dir_server_name(ServerBlock &srv)
{
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("server_name expects a token", cur_.line);
    srv.setServerName(cur_.text);
    advance();
    expect(Token::T_SEMI, "';'");
}

// La fonction dir_client_max_body_size analyse une directive "client_max_body_size"
// dans un bloc serveur.
// - Elle consomme d’abord le mot-clé "client_max_body_size" (advance).
// - Elle vérifie que le token suivant est bien un identifiant ou un nombre,
//   représentant une taille (ex: "1024", "10M", "2G").
//   Sinon, elle lève une ParseError avec le message
//   "client_max_body_size expects a number or suffixed size".
// - Elle convertit ensuite cette valeur textuelle en taille numérique (octets)
//   grâce à parseSizeWithUnit(cur_.text).
// - Puis elle enregistre cette limite dans le serveur via srv.setClientMaxBodySize(n).
// - Ensuite, elle avance pour consommer ce token de taille.
// - Enfin, elle vérifie la présence obligatoire d’un point-virgule final (';') avec expect.
// → Cette fonction permet donc de définir la taille maximale du corps d’une requête HTTP
//   que le serveur acceptera.
void ConfigParser::dir_client_max_body_size(ServerBlock &srv)
{
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_NUMBER)
        throw ParseError("client_max_body_size expects a number or suffixed size", cur_.line);
    size_t n = parseSizeWithUnit(cur_.text);
    srv.setClientMaxBodySize(n);
    advance();
    expect(Token::T_SEMI, "';'");
}

// La fonction dir_error_page analyse une directive "error_page" dans un bloc serveur.
// - Elle consomme d’abord le mot-clé "error_page" (advance).
// - Elle vérifie ensuite que le token suivant est bien un nombre, correspondant
//   au code d’erreur HTTP à traiter (ex: 404, 500).
//   Si ce n’est pas le cas, elle lève une ParseError avec le message
//   "error_page expects a numeric code".
// - Elle convertit ce texte en entier (std::atoi) pour obtenir le code d’erreur.
// - Après avoir avancé, elle vérifie que le token courant est un identifiant ou une chaîne,
//   représentant le chemin de la page d’erreur à utiliser (ex: "/errors/404.html").
//   Sinon, une ParseError est levée avec le message "error_page expects a path".
// - Elle enregistre alors cette association code → chemin via srv.setErrorPage(code, cur_.text).
// - Enfin, elle avance pour consommer le chemin, puis vérifie la présence obligatoire
//   d’un point-virgule final (';') avec expect.
// → Cette fonction permet donc de définir une page personnalisée pour un code d’erreur HTTP donné.
void ConfigParser::dir_error_page(ServerBlock &srv)
{
    advance();
    if (cur_.type != Token::T_NUMBER)
        throw ParseError("error_page expects a numeric code", cur_.line);
    int code = std::atoi(cur_.text.c_str());
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("error_page expects a path", cur_.line);
    srv.setErrorPage(code, cur_.text);
    advance();
    expect(Token::T_SEMI, "';'");
}

// La fonction dir_location analyse une directive "location" dans un bloc serveur.
// - Elle consomme d’abord le mot-clé "location" (advance).
// - Elle vérifie que le token suivant est un identifiant ou une chaîne,
//   représentant le préfixe d’URL que cette location doit gérer (ex: "/images").
//   Sinon, elle lève une ParseError avec le message "location expects a path prefix".
// - Elle mémorise ce préfixe (prefix) et avance.
// - Elle vérifie ensuite la présence obligatoire d’une accolade ouvrante '{' avec expect.
// - Elle appelle parseLocation(srv, prefix) pour analyser le contenu du bloc location
//   et construit ainsi un objet LocationBlock.
// - Après l’analyse, elle attend et consomme l’accolade fermante '}'.
// - Enfin, elle ajoute ce bloc location au serveur via srv.addLocation(loc).
// → Cette fonction permet donc de définir un bloc de configuration spécifique
//   pour un chemin ou un préfixe d’URL dans le serveur.
void ConfigParser::dir_location(ServerBlock &srv)
{
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("location expects a path prefix", cur_.line);
    std::string prefix = cur_.text;
    advance();
    expect(Token::T_LBRACE, "'{'");
    LocationBlock loc = parseLocation(srv, prefix);
    expect(Token::T_RBRACE, "'}'");
    srv.addLocation(loc);
}

// La fonction parseLocation analyse un bloc "location" du fichier de configuration.
// - Elle crée un objet LocationBlock vide (loc).
// - Elle assigne au bloc le préfixe d’URL reçu en paramètre (prefix).
// - Ensuite, elle entre dans une boucle qui continue jusqu’à rencontrer
//   une accolade fermante '}' signalant la fin du bloc.
//     * Si la fin du fichier est atteinte avant de trouver '}', une ParseError
//       est levée avec le message "Unclosed location block".
//     * Sinon, elle appelle parseLocationDirective(loc, server) pour analyser
//       et enregistrer une directive à l’intérieur de ce bloc location.
// - Quand la boucle se termine (accolade fermante trouvée), la fonction retourne loc.
// → Cette fonction construit donc un bloc "location" complet avec toutes ses directives.
LocationBlock ConfigParser::parseLocation(ServerBlock &server, const std::string &prefix)
{
    LocationBlock loc;
    loc.setPathPrefix(prefix);
    while (cur_.type != Token::T_RBRACE)
    {
        if (cur_.type == Token::T_EOF)
            throw ParseError("Unclosed location block", cur_.line);
        parseLocationDirective(loc, server);
    }
    return loc;
}

// La fonction parseLocationDirective analyse une directive à l’intérieur d’un bloc "location".
// - Le paramètre parent est présent mais n’est pas utilisé dans cette version (cast en (void)parent).
// - Elle vérifie que le token courant est bien un identifiant (nom de directive).
//   Si ce n’est pas le cas, elle lève une ParseError avec le message
//   "Expected directive name in location".
// - Ensuite, elle compare le texte du token (cur_.text) aux mots-clés connus :
//     * "root"         → appelle loc_root(loc)
//     * "methods"      → appelle loc_methods(loc)
//     * "autoindex"    → appelle loc_autoindex(loc)
//     * "index"        → appelle loc_index(loc)
//     * "upload_store" → appelle loc_upload_store(loc)
//     * "return"       → appelle loc_return(loc)
//     * "cgi_pass"     → appelle loc_cgi_pass(loc)
//   Chaque fonction spécialisée lit les arguments nécessaires et modifie le bloc loc en conséquence.
// - Si la directive n’est pas reconnue, une ParseError est levée avec un message
//   "Unknown location directive" suivi du texte rencontré.
// → Cette fonction joue le rôle de répartiteur ("dispatcher") qui oriente l’analyse
//   vers la fonction appropriée selon la directive rencontrée dans le bloc location.
void ConfigParser::parseLocationDirective(LocationBlock &loc, const ServerBlock &parent)
{
    (void)parent;
    if (cur_.type != Token::T_IDENT)
        throw ParseError("Expected directive name in location", cur_.line);

    if (cur_.text == "root")
    {
        loc_root(loc);
        return;
    }
    if (cur_.text == "methods")
    {
        loc_methods(loc);
        return;
    }
    if (cur_.text == "autoindex")
    {
        loc_autoindex(loc);
        return;
    }
    if (cur_.text == "index")
    {
        loc_index(loc);
        return;
    }
    if (cur_.text == "upload_store")
    {
        loc_upload_store(loc);
        return;
    }
    if (cur_.text == "return")
    {
        loc_return(loc);
        return;
    }
    if (cur_.text == "cgi_pass")
    {
        loc_cgi_pass(loc);
        return;
    }

    throw ParseError("Unknown location directive: " + cur_.text, cur_.line);
}

// La fonction loc_root analyse une directive "root" à l’intérieur d’un bloc "location".
// - Elle consomme d’abord le mot-clé "root" (advance).
// - Elle vérifie que le token suivant est un identifiant ou une chaîne,
//   représentant le chemin du répertoire racine associé à cette location.
//   Sinon, elle lève une ParseError avec le message "root expects a path".
// - Si le chemin est valide, elle l’enregistre dans le bloc location via loc.setRoot(cur_.text).
// - Ensuite, elle avance pour consommer le token du chemin.
// - Enfin, elle vérifie la présence obligatoire d’un point-virgule final (';') avec expect.
// → Cette fonction permet donc de définir un répertoire racine spécifique pour une location donnée.
void ConfigParser::loc_root(LocationBlock &loc)
{
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("root expects a path", cur_.line);
    loc.setRoot(cur_.text);
    advance();
    expect(Token::T_SEMI, "';'");
}

// La fonction loc_methods analyse la directive "methods" dans un bloc "location".
// - Elle consomme d’abord le mot-clé "methods" (advance).
// - Elle vérifie que le token suivant est un identifiant ; sinon, elle lève
//   une ParseError avec le message "methods expects a list (GET/POST/DELETE)".
// - Tant que le token courant est un identifiant, elle ajoute cette méthode
//   (ex. GET, POST, DELETE) au bloc location via loc.addMethod(cur_.text), puis avance.
// - Lorsque la liste se termine, elle exige un point-virgule final (';') avec expect.
// → Cette fonction permet de définir l’ensemble des méthodes HTTP autorisées
//   pour la location courante.
void ConfigParser::loc_methods(LocationBlock &loc)
{
    advance();
    if (cur_.type != Token::T_IDENT)
        throw ParseError("methods expects a list (GET/POST/DELETE)", cur_.line);
    while (cur_.type == Token::T_IDENT)
    {
        loc.addMethod(cur_.text);
        advance();
    }
    expect(Token::T_SEMI, "';'");
}

// La fonction loc_autoindex analyse la directive "autoindex" dans un bloc "location".
// - Elle consomme d’abord le mot-clé "autoindex" (advance).
// - Elle vérifie que le token suivant est un identifiant ; sinon, elle lève
//   une ParseError avec le message "autoindex expects 'on' or 'off'".
// - Si le texte du token est "on", elle active l’autoindex via loc.setAutoIndex(true).
// - Si le texte du token est "off", elle désactive l’autoindex via loc.setAutoIndex(false).
// - Si la valeur est différente, une ParseError est levée avec le message
//   "autoindex value must be 'on' or 'off'".
// - Elle avance ensuite pour consommer le token, puis exige la présence d’un
//   point-virgule final (';') avec expect.
// → Cette fonction permet donc de configurer l’option d’indexation automatique
//   d’un répertoire pour une location donnée.
void ConfigParser::loc_autoindex(LocationBlock &loc)
{
    advance();
    if (cur_.type != Token::T_IDENT)
        throw ParseError("autoindex expects 'on' or 'off'", cur_.line);
    if (cur_.text == "on")
        loc.setAutoIndex(true);
    else if (cur_.text == "off")
        loc.setAutoIndex(false);
    else throw ParseError("autoindex value must be 'on' or 'off'", cur_.line);
    advance();
    expect(Token::T_SEMI, "';'");
}

// La fonction loc_index analyse la directive "index" dans un bloc "location".
// - Elle consomme d’abord le mot-clé "index" (advance).
// - Elle vérifie que le token suivant est un identifiant ou une chaîne ;
//   sinon, elle lève une ParseError avec le message
//   "index expects at least one filename".
// - Tant que le token courant est un identifiant ou une chaîne, elle ajoute
//   ce nom de fichier d’index (ex: "index.html", "default.html") au bloc location
//   via loc.addIndex(cur_.text), puis avance.
// - Lorsque la liste se termine, elle exige la présence obligatoire
//   d’un point-virgule final (';') avec expect.
// → Cette fonction permet donc de définir une ou plusieurs pages d’index
//   spécifiques à la location.
void ConfigParser::loc_index(LocationBlock &loc)
{
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("index expects at least one filename", cur_.line);
    while (cur_.type == Token::T_IDENT || cur_.type == Token::T_STRING)
    {
        loc.addIndex(cur_.text);
        advance();
    }
    expect(Token::T_SEMI, "';'");
}

// La fonction loc_upload_store analyse la directive "upload_store" dans un bloc "location".
// - Elle consomme d’abord le mot-clé "upload_store" (advance).
// - Elle vérifie que le token suivant est un identifiant ou une chaîne,
//   représentant le chemin du répertoire de stockage des fichiers uploadés.
//   Si ce n’est pas le cas, elle lève une ParseError avec le message
//   "upload_store expects a path".
// - Si le chemin est valide, elle l’enregistre dans le bloc location via loc.setUploadStore(cur_.text).
// - Ensuite, elle avance pour consommer ce token de chemin.
// - Enfin, elle exige la présence d’un point-virgule final (';') avec expect.
// → Cette fonction permet donc de définir où les fichiers envoyés par les clients
//   seront enregistrés pour cette location.
void ConfigParser::loc_upload_store(LocationBlock &loc)
{
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("upload_store expects a path", cur_.line);
    loc.setUploadStore(cur_.text);
    advance();
    expect(Token::T_SEMI, "';'");
}

// La fonction loc_return analyse la directive "return" dans un bloc "location".
// - Elle consomme d’abord le mot-clé "return" (advance).
// - Elle vérifie que le token suivant est un nombre ou un identifiant,
//   correspondant au code de statut HTTP (ex: 301, 302, 404).
//   Sinon, une ParseError est levée avec le message "return expects a status code".
// - Elle mémorise ce code sous forme de chaîne et avance.
// - Si le token suivant est immédiatement un point-virgule (';'),
//   alors la directive "return" ne spécifie que le code :
//     * un redirection/retour simple est enregistré via loc.setRedirect(code, "").
//     * puis elle consomme le ';' et retourne.
// - Sinon, elle attend un identifiant ou une chaîne représentant la cible
//   (chemin ou URL de redirection).
//   Si ce n’est pas le cas, elle lève une ParseError avec le message
//   "return expects a target path or URL".
// - Elle mémorise cette cible (target), avance, et appelle loc.setRedirect(code, target).
// - Enfin, elle vérifie la présence obligatoire d’un ';' final avec expect.
// → Cette fonction permet donc de configurer une directive "return" qui peut être :
//   * soit un simple code de statut,
//   * soit un code accompagné d’une URL ou d’un chemin de redirection.
void ConfigParser::loc_return(LocationBlock &loc)
{
    advance();
    if (cur_.type != Token::T_NUMBER && cur_.type != Token::T_IDENT)
        throw ParseError("return expects a status code", cur_.line);
    std::string code = cur_.text;
    advance();
    if (cur_.type == Token::T_SEMI)
    {
        loc.setRedirect(code, "");
        advance();
        return;
    }
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("return expects a target path or URL", cur_.line);
    std::string target = cur_.text;
    advance();
    loc.setRedirect(code, target);
    expect(Token::T_SEMI, "';'");
}

// La fonction loc_cgi_pass analyse la directive "cgi_pass" dans un bloc "location".
// - Elle consomme d’abord le mot-clé "cgi_pass" (advance).
// - Elle vérifie que le token suivant est un identifiant, représentant une extension
//   de fichier (ex: ".py", ".php"). Sinon, elle lève une ParseError avec le message
//   "cgi_pass expects an extension (e.g. .py)".
// - Elle mémorise cette extension dans ext, puis avance.
// - Elle vérifie ensuite que le token courant est un identifiant ou une chaîne,
//   correspondant au chemin de l’interpréteur à utiliser (ex: "/usr/bin/python3").
//   Sinon, une ParseError est levée avec le message
//   "cgi_pass expects an interpreter path".
// - Elle mémorise ce chemin dans execPath, puis avance.
// - Elle associe l’extension et l’interpréteur via loc.mapCgi(ext, execPath).
// - Enfin, elle vérifie la présence obligatoire d’un point-virgule final (';') avec expect.
// → Cette fonction permet donc de définir quel interpréteur doit être utilisé
//   pour exécuter les scripts d’une certaine extension dans la location.
void ConfigParser::loc_cgi_pass(LocationBlock &loc)
{
    advance();
    if (cur_.type != Token::T_IDENT)
        throw ParseError("cgi_pass expects an extension (e.g. .py)", cur_.line);
    std::string ext = cur_.text;
    advance();
    if (cur_.type != Token::T_IDENT && cur_.type != Token::T_STRING)
        throw ParseError("cgi_pass expects an interpreter path", cur_.line);
    std::string execPath = cur_.text;
    advance();
    loc.mapCgi(ext, execPath);
    expect(Token::T_SEMI, "';'");
}

// ------------------ Helpers ------------------
// La fonction parseSizeWithUnit convertit une chaîne représentant une taille
// (optionnellement suffixée par une unité) en un entier de type size_t.
// - Si la chaîne est vide, elle retourne 0.
// - Elle lit d’abord la partie numérique au début de la chaîne (boucle sur les chiffres).
// - Elle sépare ensuite la valeur numérique (num) du suffixe éventuel (suf).
// - Elle convertit la partie numérique en entier (base) avec strtoul.
// - Si aucun suffixe n’est présent, elle retourne base directement.
// - Sinon, elle interprète le suffixe comme une unité de taille :
//     * "K" ou "k"  → base * 1024 (kilooctets)
//     * "M" ou "m"  → base * 1024² (mégaoctets)
//     * "G" ou "g"  → base * 1024³ (gigaoctets)
//     * "KB"/"kb"   → base * 1024
//     * "MB"/"mb"   → base * 1024²
//     * "GB"/"gb"   → base * 1024³
// - Si le suffixe n’est pas reconnu, elle retourne simplement la valeur numérique (base).
// → Cette fonction permet donc de manipuler des tailles exprimées
//   en nombres simples ou avec des unités classiques (K, M, G).
size_t ConfigParser::parseSizeWithUnit(const std::string &s)
{
    if (s.empty()) return 0;
    size_t i = 0;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
        ++i;
    std::string num = s.substr(0, i);
    std::string suf = s.substr(i);
    size_t base = static_cast<size_t>(std::strtoul(num.c_str(), 0, 10));
    if (suf.empty())
        return base;
    if (suf == "K" || suf == "k")
        return base * 1024u;
    if (suf == "M" || suf == "m")
        return base * 1024u * 1024u;
    if (suf == "G" || suf == "g")
        return base * 1024u * 1024u * 1024u;
    if (suf == "KB" || suf == "kb")
        return base * 1024u;
    if (suf == "MB" || suf == "mb")
        return base * 1024u * 1024u;
    if (suf == "GB" || suf == "gb")
        return base * 1024u * 1024u * 1024u;
    return base;
}

// La fonction splitHostPort découpe une chaîne de type "host:port" en deux parties distinctes.
// - Elle cherche le dernier caractère ':' dans la chaîne.
// - Si aucun ':' n’est trouvé :
//     * la chaîne entière est interprétée comme un numéro de port,
//     * l’hôte est par défaut "0.0.0.0" (écoute sur toutes les interfaces).
// - Sinon :
//     * la partie avant ':' est affectée à host,
//     * la partie après ':' est affectée à p (le port sous forme de chaîne).
// - Si host est vide ou égal à "*", il est remplacé par "0.0.0.0".
// - Le port est converti en entier avec std::atoi et affecté à port.
// → Cette fonction permet donc de gérer aussi bien une valeur simple ("8080")
//   qu’une valeur complète avec adresse et port ("127.0.0.1:8080").
void ConfigParser::splitHostPort(const std::string &s, std::string &host, int &port)
{
    std::string h = s;
    std::string p;
    std::string::size_type pos = h.rfind(':');
    if (pos == std::string::npos)
    {
        host = "0.0.0.0";
        port = std::atoi(h.c_str());
        return;
    }
    host = h.substr(0, pos);
    p = h.substr(pos + 1);
    if (host == "" || host == "*") host = "0.0.0.0";
    port = std::atoi(p.c_str());
}
