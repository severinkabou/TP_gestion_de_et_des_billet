// ============================================================
//  Agence de voyage de Cotonou - Gestion des vols et reservations
//  Application console C++ (POO) - compiler avec : g++ -std=c++17
// ============================================================
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;

// ------------------------------------------------------------
//  Enumerations
// ------------------------------------------------------------
enum class StatutReservation { EN_ATTENTE, VALIDEE, ANNULEE };
enum class StatutRemboursement { AUCUN, A_REMBOURSER, REMBOURSE };

string versTexte(StatutReservation s) {
    switch (s) {
        case StatutReservation::EN_ATTENTE: return "EN_ATTENTE";
        case StatutReservation::VALIDEE:    return "VALIDEE";
        default:                            return "ANNULEE";
    }
}

string versTexte(StatutRemboursement s) {
    switch (s) {
        case StatutRemboursement::AUCUN:        return "AUCUN";
        case StatutRemboursement::A_REMBOURSER: return "A_REMBOURSER";
        default:                                return "REMBOURSE";
    }
}

// ------------------------------------------------------------
//  Utilitaires de texte et de saisie (gestion des saisies interdites)
// ------------------------------------------------------------
string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string enMinuscules(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return (char)tolower(c); });
    return s;
}

string enMajuscules(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return (char)toupper(c); });
    return s;
}

string dateDuJour() {
    time_t t = time(nullptr);
    char buf[11];
    strftime(buf, sizeof(buf), "%d/%m/%Y", localtime(&t));
    return string(buf);
}

bool dateValide(const string& d) {            // JJ/MM/AAAA
    if (d.size() != 10 || d[2] != '/' || d[5] != '/') return false;
    for (size_t i = 0; i < d.size(); ++i) {
        if (i == 2 || i == 5) continue;
        if (!isdigit((unsigned char)d[i])) return false;
    }
    int jour = stoi(d.substr(0, 2));
    int mois = stoi(d.substr(3, 2));
    int an   = stoi(d.substr(6, 4));
    return jour >= 1 && jour <= 31 && mois >= 1 && mois <= 12 &&
           an >= 2000 && an <= 2100;
}

bool heureValide(const string& h) {           // HH:MM
    if (h.size() != 5 || h[2] != ':') return false;
    if (!isdigit((unsigned char)h[0]) || !isdigit((unsigned char)h[1]) ||
        !isdigit((unsigned char)h[3]) || !isdigit((unsigned char)h[4]))
        return false;
    int hh = stoi(h.substr(0, 2));
    int mm = stoi(h.substr(3, 2));
    return hh <= 23 && mm <= 59;
}

bool telephoneValide(const string& t) {
    int chiffres = 0;
    for (char c : t) {
        if (isdigit((unsigned char)c)) ++chiffres;
        else if (c != '+' && c != ' ') return false;
    }
    return chiffres >= 8;
}

bool emailValide(const string& e) {
    size_t at = e.find('@');
    if (at == string::npos || at == 0) return false;
    size_t point = e.find('.', at);
    return point != string::npos && point > at + 1 && point < e.size() - 1 &&
           e.find(' ') == string::npos;
}

string lireLigne(const string& invite) {
    cout << invite;
    string s;
    if (!getline(cin, s)) {                   // fin de flux (Ctrl+D / Ctrl+Z)
        cout << "\nFin de la saisie. Fermeture du programme.\n";
        exit(0);
    }
    return s;
}

string lireTexte(const string& invite) {
    while (true) {
        string s = trim(lireLigne(invite));
        if (!s.empty()) return s;
        cout << "  [Erreur] Ce champ est obligatoire.\n";
    }
}

int lireEntier(const string& invite, int mini, int maxi) {
    while (true) {
        string s = trim(lireLigne(invite));
        try {
            size_t pos = 0;
            long v = stol(s, &pos);
            if (pos == s.size() && v >= mini && v <= maxi) return (int)v;
        } catch (...) {
        }
        cout << "  [Erreur] Entrez un nombre entier entre " << mini
             << " et " << maxi << ".\n";
    }
}

double lireReel(const string& invite) {
    while (true) {
        string s = trim(lireLigne(invite));
        try {
            size_t pos = 0;
            double v = stod(s, &pos);
            if (pos == s.size() && v > 0) return v;
        } catch (...) {
        }
        cout << "  [Erreur] Entrez un nombre strictement positif.\n";
    }
}

bool lireOuiNon(const string& invite) {
    while (true) {
        string s = enMinuscules(trim(lireLigne(invite + " (o/n) : ")));
        if (s == "o" || s == "oui") return true;
        if (s == "n" || s == "non") return false;
        cout << "  [Erreur] Repondez par o ou n.\n";
    }
}

// ------------------------------------------------------------
//  Declarations anticipees
// ------------------------------------------------------------
class Reservation;
class Vol;

// ------------------------------------------------------------
//  Classe Paiement
// ------------------------------------------------------------
class Paiement {
private:
    int idPaiement;
    double montant;
    string datePaiement;
    string mode;
    StatutRemboursement statutRemboursement;

public:
    Paiement(int id, double montant, const string& date, const string& mode)
        : idPaiement(id), montant(montant), datePaiement(date), mode(mode),
          statutRemboursement(StatutRemboursement::AUCUN) {}

    int getId() const { return idPaiement; }
    double getMontant() const { return montant; }
    const string& getDate() const { return datePaiement; }
    const string& getMode() const { return mode; }
    StatutRemboursement getStatutRemboursement() const {
        return statutRemboursement;
    }

    // Appele lorsqu'une reservation payee est annulee
    void marquerARembourser() {
        if (statutRemboursement == StatutRemboursement::AUCUN)
            statutRemboursement = StatutRemboursement::A_REMBOURSER;
    }

    bool rembourser() {
        if (statutRemboursement != StatutRemboursement::A_REMBOURSER)
            return false;
        statutRemboursement = StatutRemboursement::REMBOURSE;
        return true;
    }
};

// ------------------------------------------------------------
//  Classe Passager
// ------------------------------------------------------------
class Passager {
private:
    int id;
    string nom, prenom, telephone, email, numPasseport;
    vector<Reservation*> reservations;   // association 1 -- 0..*

public:
    Passager(int id, const string& nom, const string& prenom,
             const string& telephone, const string& email,
             const string& numPasseport)
        : id(id), nom(nom), prenom(prenom), telephone(telephone),
          email(email), numPasseport(numPasseport) {}

    int getId() const { return id; }
    const string& getNom() const { return nom; }
    const string& getPrenom() const { return prenom; }
    const string& getTelephone() const { return telephone; }
    const string& getEmail() const { return email; }
    const string& getNumPasseport() const { return numPasseport; }
    const vector<Reservation*>& getReservations() const { return reservations; }

    void ajouterReservation(Reservation* r) { reservations.push_back(r); }
    bool aReservationActive(const Vol* v) const;   // defini plus bas

    string nomComplet() const { return prenom + " " + nom; }

    void afficher() const {
        cout << "  Passager #" << id << " : " << nomComplet()
             << " | Tel : " << telephone << " | Email : " << email
             << " | Passeport : " << numPasseport << "\n";
    }
};

// ------------------------------------------------------------
//  Classe Vol
// ------------------------------------------------------------
class Vol {
private:
    string numeroVol, villeDepart, villeArrivee, dateDepart, heureDepart;
    int capacite;
    double prix;
    vector<Reservation*> reservations;   // association 1 -- 0..*

public:
    Vol(const string& numero, const string& depart, const string& arrivee,
        const string& date, const string& heure, int capacite, double prix)
        : numeroVol(numero), villeDepart(depart), villeArrivee(arrivee),
          dateDepart(date), heureDepart(heure), capacite(capacite),
          prix(prix) {}

    const string& getNumeroVol() const { return numeroVol; }
    const string& getVilleDepart() const { return villeDepart; }
    const string& getVilleArrivee() const { return villeArrivee; }
    const string& getDateDepart() const { return dateDepart; }
    const string& getHeureDepart() const { return heureDepart; }
    int getCapacite() const { return capacite; }
    double getPrix() const { return prix; }

    int placesOccupees() const;                    // defini plus bas
    int placesDisponibles() const { return capacite - placesOccupees(); }
    bool estComplet() const { return placesDisponibles() <= 0; }
    int prochainePlaceLibre() const;               // defini plus bas

    bool ajouterReservation(Reservation* r) {
        if (estComplet()) return false;
        reservations.push_back(r);
        return true;
    }

    static void afficherEntete() {
        cout << left << setw(8) << "Vol" << setw(14) << "Depart"
             << setw(14) << "Arrivee" << setw(12) << "Date"
             << setw(7) << "Heure" << setw(9) << "Places"
             << "Prix (FCFA)\n";
        cout << string(77, '-') << "\n";
    }

    void afficherLigne() const {
        cout << left << setw(8) << numeroVol << setw(14) << villeDepart
             << setw(14) << villeArrivee << setw(12) << dateDepart
             << setw(7) << heureDepart
             << setw(9) << (to_string(placesDisponibles()) + "/" +
                            to_string(capacite))
             << fixed << setprecision(0) << prix << "\n";
    }
};

// ------------------------------------------------------------
//  Classe Reservation
// ------------------------------------------------------------
class Reservation {
private:
    int idReservation;
    string dateReservation;
    int numPlace;
    StatutReservation statut;
    Passager* passager;                  // exactement 1 passager
    Vol* vol;                            // exactement 1 vol
    unique_ptr<Paiement> paiement;       // 0..1 paiement

public:
    Reservation(int id, Passager* p, Vol* v, int place)
        : idReservation(id), dateReservation(dateDuJour()), numPlace(place),
          statut(StatutReservation::EN_ATTENTE), passager(p), vol(v) {}

    int getId() const { return idReservation; }
    const string& getDateReservation() const { return dateReservation; }
    int getNumPlace() const { return numPlace; }
    StatutReservation getStatut() const { return statut; }
    Passager* getPassager() const { return passager; }
    Vol* getVol() const { return vol; }
    const Paiement* getPaiement() const { return paiement.get(); }
    Paiement* getPaiement() { return paiement.get(); }

    // Regle 5 : une reservation annulee n'est plus payable
    bool estPayable() const {
        return statut == StatutReservation::EN_ATTENTE && !paiement;
    }

    // Regle 6 : la place est definitive apres validation (paiement enregistre)
    bool enregistrerPaiement(const Paiement& p) {
        if (!estPayable()) return false;
        paiement = make_unique<Paiement>(p);
        statut = StatutReservation::VALIDEE;
        return true;
    }

    // Regle 7 : si la reservation etait payee, le remboursement est prevu
    bool annuler() {
        if (statut == StatutReservation::ANNULEE) return false;
        if (paiement) paiement->marquerARembourser();
        statut = StatutReservation::ANNULEE;
        return true;
    }

    void afficherLigne() const {
        cout << left << setw(6) << idReservation
             << setw(22) << passager->nomComplet().substr(0, 21)
             << setw(8) << vol->getNumeroVol()
             << setw(8) << numPlace
             << setw(12) << versTexte(statut)
             << (paiement ? versTexte(paiement->getStatutRemboursement())
                          : string("-"))
             << "\n";
    }

    void afficherDetails() const {
        cout << "\n=== Reservation #" << idReservation << " ===\n";
        cout << "  Date de reservation : " << dateReservation << "\n";
        cout << "  Statut              : " << versTexte(statut) << "\n";
        cout << "  Place               : " << numPlace
             << (statut == StatutReservation::VALIDEE
                     ? " (definitivement reservee)"
                     : statut == StatutReservation::EN_ATTENTE
                           ? " (provisoire, en attente de paiement)"
                           : " (liberee)")
             << "\n";
        cout << "  Passager            : " << passager->nomComplet()
             << " (id " << passager->getId() << ", passeport "
             << passager->getNumPasseport() << ")\n";
        cout << "  Vol                 : " << vol->getNumeroVol() << " "
             << vol->getVilleDepart() << " -> " << vol->getVilleArrivee()
             << ", le " << vol->getDateDepart() << " a "
             << vol->getHeureDepart() << "\n";
        cout << "  Prix du billet      : " << fixed << setprecision(0)
             << vol->getPrix() << " FCFA\n";
        if (paiement) {
            cout << "  Paiement            : #" << paiement->getId() << ", "
                 << paiement->getMontant() << " FCFA, "
                 << paiement->getMode() << ", le " << paiement->getDate()
                 << "\n";
            cout << "  Remboursement       : "
                 << versTexte(paiement->getStatutRemboursement()) << "\n";
        } else {
            cout << "  Paiement            : aucun\n";
        }
    }
};

// ------------------------------------------------------------
//  Methodes definies apres Reservation
// ------------------------------------------------------------
bool Passager::aReservationActive(const Vol* v) const {
    for (const Reservation* r : reservations)
        if (r->getVol() == v && r->getStatut() != StatutReservation::ANNULEE)
            return true;
    return false;
}

int Vol::placesOccupees() const {
    int n = 0;
    for (const Reservation* r : reservations)
        if (r->getStatut() != StatutReservation::ANNULEE) ++n;
    return n;
}

int Vol::prochainePlaceLibre() const {
    for (int place = 1; place <= capacite; ++place) {
        bool prise = false;
        for (const Reservation* r : reservations)
            if (r->getStatut() != StatutReservation::ANNULEE &&
                r->getNumPlace() == place) {
                prise = true;
                break;
            }
        if (!prise) return place;
    }
    return -1;
}

// ------------------------------------------------------------
//  Classe Agence : gere les collections (vector) et les regles
// ------------------------------------------------------------
class Agence {
private:
    string nom, ville;
    vector<unique_ptr<Passager>> passagers;
    vector<unique_ptr<Vol>> vols;
    vector<unique_ptr<Reservation>> reservations;
    int prochainIdPassager = 1;
    int prochainIdReservation = 1;
    int prochainIdPaiement = 1;

public:
    Agence(const string& nom, const string& ville) : nom(nom), ville(ville) {}
    Agence(const Agence&) = delete;
    Agence& operator=(const Agence&) = delete;

    const string& getNom() const { return nom; }
    const string& getVille() const { return ville; }
    bool aDesPassagers() const { return !passagers.empty(); }
    bool aDesVols() const { return !vols.empty(); }
    const vector<unique_ptr<Reservation>>& getReservations() const {
        return reservations;
    }

    Passager* enregistrerPassager(const string& nomP, const string& prenom,
                                  const string& tel, const string& email,
                                  const string& passeport) {
        for (const auto& p : passagers)
            if (p->getNumPasseport() == passeport) return nullptr;
        passagers.push_back(make_unique<Passager>(
            prochainIdPassager++, nomP, prenom, tel, email, passeport));
        return passagers.back().get();
    }

    Vol* creerVol(const string& numero, const string& depart,
                  const string& arrivee, const string& date,
                  const string& heure, int capacite, double prix) {
        if (trouverVol(numero)) return nullptr;
        vols.push_back(make_unique<Vol>(numero, depart, arrivee, date, heure,
                                        capacite, prix));
        return vols.back().get();
    }

    Passager* trouverPassager(int id) const {
        for (const auto& p : passagers)
            if (p->getId() == id) return p.get();
        return nullptr;
    }

    Vol* trouverVol(const string& numero) const {
        string n = enMajuscules(numero);
        for (const auto& v : vols)
            if (v->getNumeroVol() == n) return v.get();
        return nullptr;
    }

    Reservation* trouverReservation(int id) const {
        for (const auto& r : reservations)
            if (r->getId() == id) return r.get();
        return nullptr;
    }

    vector<Vol*> volsDisponibles() const {
        vector<Vol*> res;
        for (const auto& v : vols)
            if (!v->estComplet()) res.push_back(v.get());
        return res;
    }

    vector<Vol*> rechercherVols(const string& depart,
                                const string& arrivee) const {
        vector<Vol*> res;
        string d = enMinuscules(depart), a = enMinuscules(arrivee);
        for (const auto& v : vols) {
            bool okD = d.empty() ||
                       enMinuscules(v->getVilleDepart()).find(d) != string::npos;
            bool okA = a.empty() ||
                       enMinuscules(v->getVilleArrivee()).find(a) != string::npos;
            if (okD && okA) res.push_back(v.get());
        }
        return res;
    }

    // Regles 1 (passager existant), 2 (capacite), 3 (pas de doublon), 4
    Reservation* reserver(Passager* p, Vol* v, string& erreur) {
        if (!p) { erreur = "Passager non enregistre."; return nullptr; }
        if (!v) { erreur = "Vol introuvable."; return nullptr; }
        if (v->estComplet()) {
            erreur = "Le vol " + v->getNumeroVol() + " est complet.";
            return nullptr;
        }
        if (p->aReservationActive(v)) {
            erreur = "Ce passager a deja une reservation pour ce vol.";
            return nullptr;
        }
        reservations.push_back(make_unique<Reservation>(
            prochainIdReservation++, p, v, v->prochainePlaceLibre()));
        Reservation* r = reservations.back().get();
        v->ajouterReservation(r);
        p->ajouterReservation(r);
        return r;
    }

    // Regles 5 et 6
    bool payer(Reservation* r, double montant, const string& mode,
               string& erreur) {
        if (!r) { erreur = "Reservation introuvable."; return false; }
        if (r->getStatut() == StatutReservation::ANNULEE) {
            erreur = "Une reservation annulee ne peut pas etre payee.";
            return false;
        }
        if (!r->estPayable()) {
            erreur = "Cette reservation est deja payee et validee.";
            return false;
        }
        if (montant != r->getVol()->getPrix()) {
            erreur = "Montant incorrect : le prix du billet est de " +
                     to_string((long)r->getVol()->getPrix()) + " FCFA.";
            return false;
        }
        Paiement p(prochainIdPaiement++, montant, dateDuJour(), mode);
        return r->enregistrerPaiement(p);
    }

    // Regle 7
    bool annuler(Reservation* r, string& erreur) {
        if (!r) { erreur = "Reservation introuvable."; return false; }
        if (r->getStatut() == StatutReservation::ANNULEE) {
            erreur = "Cette reservation est deja annulee.";
            return false;
        }
        return r->annuler();
    }
};

// ------------------------------------------------------------
//  Interface console
// ------------------------------------------------------------
void afficherMenu() {
    cout << "\n=========== AGENCE DE VOYAGE - COTONOU ===========\n"
         << " 1. Enregistrer un passager\n"
         << " 2. Ajouter un vol\n"
         << " 3. Afficher les vols disponibles\n"
         << " 4. Rechercher un vol\n"
         << " 5. Effectuer une reservation\n"
         << " 6. Enregistrer un paiement\n"
         << " 7. Annuler une reservation\n"
         << " 8. Consulter les reservations\n"
         << " 9. Afficher les informations d'une reservation\n"
         << "10. Quitter\n"
         << "==================================================\n";
}

void enregistrerPassager(Agence& agence) {
    cout << "\n--- Enregistrement d'un passager ---\n";
    string nom = lireTexte("Nom : ");
    string prenom = lireTexte("Prenom : ");
    string tel;
    while (true) {
        tel = lireTexte("Telephone : ");
        if (telephoneValide(tel)) break;
        cout << "  [Erreur] Telephone invalide (8 chiffres minimum).\n";
    }
    string email;
    while (true) {
        email = lireTexte("Email : ");
        if (emailValide(email)) break;
        cout << "  [Erreur] Email invalide.\n";
    }
    string passeport = enMajuscules(lireTexte("Numero de passeport : "));
    Passager* p = agence.enregistrerPassager(nom, prenom, tel, email, passeport);
    if (!p)
        cout << "  [Refuse] Un passager avec ce passeport existe deja.\n";
    else
        cout << "  Passager enregistre. Identifiant : " << p->getId() << "\n";
}

void ajouterVol(Agence& agence) {
    cout << "\n--- Ajout d'un vol ---\n";
    string numero = enMajuscules(lireTexte("Numero du vol : "));
    if (agence.trouverVol(numero)) {
        cout << "  [Refuse] Ce numero de vol existe deja.\n";
        return;
    }
    string depart = lireTexte("Ville de depart : ");
    string arrivee = lireTexte("Ville d'arrivee : ");
    if (enMinuscules(depart) == enMinuscules(arrivee)) {
        cout << "  [Refuse] Depart et arrivee doivent etre differents.\n";
        return;
    }
    string date;
    while (true) {
        date = lireTexte("Date de depart (JJ/MM/AAAA) : ");
        if (dateValide(date)) break;
        cout << "  [Erreur] Format de date invalide.\n";
    }
    string heure;
    while (true) {
        heure = lireTexte("Heure de depart (HH:MM) : ");
        if (heureValide(heure)) break;
        cout << "  [Erreur] Format d'heure invalide.\n";
    }
    int capacite = lireEntier("Capacite (1 a 600) : ", 1, 600);
    double prix = lireReel("Prix du billet (FCFA) : ");
    agence.creerVol(numero, depart, arrivee, date, heure, capacite, prix);
    cout << "  Vol " << numero << " ajoute.\n";
}

void afficherListeVols(const vector<Vol*>& liste) {
    Vol::afficherEntete();
    for (const Vol* v : liste) v->afficherLigne();
}

void afficherVolsDisponibles(const Agence& agence) {
    cout << "\n--- Vols disponibles ---\n";
    vector<Vol*> liste = agence.volsDisponibles();
    if (liste.empty()) {
        cout << "  Aucun vol disponible.\n";
        return;
    }
    afficherListeVols(liste);
}

void rechercherVol(const Agence& agence) {
    cout << "\n--- Recherche d'un vol ---\n"
         << "1. Par numero de vol\n2. Par villes (depart / arrivee)\n";
    int choix = lireEntier("Votre choix : ", 1, 2);
    vector<Vol*> res;
    if (choix == 1) {
        Vol* v = agence.trouverVol(trim(lireTexte("Numero du vol : ")));
        if (v) res.push_back(v);
    } else {
        string d = trim(lireLigne("Ville de depart (vide = toutes) : "));
        string a = trim(lireLigne("Ville d'arrivee (vide = toutes) : "));
        res = agence.rechercherVols(d, a);
    }
    if (res.empty()) {
        cout << "  Aucun vol trouve.\n";
        return;
    }
    afficherListeVols(res);
}

void effectuerReservation(Agence& agence) {
    cout << "\n--- Nouvelle reservation ---\n";
    if (!agence.aDesPassagers()) {
        cout << "  [Refuse] Aucun passager enregistre. "
                "Enregistrez d'abord un passager.\n";
        return;
    }
    if (!agence.aDesVols()) {
        cout << "  [Refuse] Aucun vol n'existe.\n";
        return;
    }
    int idP = lireEntier("Identifiant du passager : ", 1, 1000000);
    Passager* p = agence.trouverPassager(idP);
    if (!p) {
        cout << "  [Refuse] Passager non enregistre : "
                "il doit etre enregistre avant de reserver.\n";
        return;
    }
    afficherVolsDisponibles(agence);
    Vol* v = agence.trouverVol(trim(lireTexte("Numero du vol a reserver : ")));
    string erreur;
    Reservation* r = agence.reserver(p, v, erreur);
    if (!r) {
        cout << "  [Refuse] " << erreur << "\n";
        return;
    }
    cout << "  Reservation #" << r->getId() << " creee (place "
         << r->getNumPlace() << ", statut EN_ATTENTE).\n"
         << "  La place sera definitive apres paiement et validation.\n";
}

void enregistrerPaiement(Agence& agence) {
    cout << "\n--- Enregistrement d'un paiement ---\n";
    int id = lireEntier("Numero de la reservation : ", 1, 1000000);
    Reservation* r = agence.trouverReservation(id);
    if (!r) {
        cout << "  [Refuse] Reservation introuvable.\n";
        return;
    }
    cout << "  Prix du billet : " << fixed << setprecision(0)
         << r->getVol()->getPrix() << " FCFA\n";
    if (r->getStatut() == StatutReservation::ANNULEE) {
        cout << "  [Refuse] Une reservation annulee ne peut pas etre payee.\n";
        return;
    }
    if (!r->estPayable()) {
        cout << "  [Refuse] Cette reservation est deja payee.\n";
        return;
    }
    double montant = lireReel("Montant verse (FCFA) : ");
    cout << "Mode : 1. Especes  2. Mobile Money  3. Carte bancaire\n";
    int m = lireEntier("Votre choix : ", 1, 3);
    string mode = (m == 1) ? "Especes" : (m == 2) ? "Mobile Money"
                                                  : "Carte bancaire";
    string erreur;
    if (agence.payer(r, montant, mode, erreur))
        cout << "  Paiement enregistre. Reservation VALIDEE : la place "
             << r->getNumPlace() << " est definitivement reservee.\n";
    else
        cout << "  [Refuse] " << erreur << "\n";
}

void annulerReservation(Agence& agence) {
    cout << "\n--- Annulation d'une reservation ---\n";
    int id = lireEntier("Numero de la reservation : ", 1, 1000000);
    Reservation* r = agence.trouverReservation(id);
    if (!r) {
        cout << "  [Refuse] Reservation introuvable.\n";
        return;
    }
    if (r->getStatut() == StatutReservation::ANNULEE) {
        cout << "  [Refuse] Cette reservation est deja annulee.\n";
        return;
    }
    if (!lireOuiNon("Confirmer l'annulation de la reservation #" +
                    to_string(id) + " ?")) {
        cout << "  Annulation abandonnee.\n";
        return;
    }
    string erreur;
    if (!agence.annuler(r, erreur)) {
        cout << "  [Refuse] " << erreur << "\n";
        return;
    }
    cout << "  Reservation annulee. La place est liberee.\n";
    Paiement* p = r->getPaiement();
    if (p) {
        cout << "  Cette reservation etait payee : remboursement de "
             << fixed << setprecision(0) << p->getMontant()
             << " FCFA a traiter (statut A_REMBOURSER).\n";
        if (lireOuiNon("Rembourser le passager maintenant ?")) {
            p->rembourser();
            cout << "  Remboursement effectue (statut REMBOURSE).\n";
        }
    }
}

void consulterReservations(const Agence& agence) {
    cout << "\n--- Liste des reservations ---\n";
    if (agence.getReservations().empty()) {
        cout << "  Aucune reservation.\n";
        return;
    }
    cout << left << setw(6) << "ID" << setw(22) << "Passager" << setw(8)
         << "Vol" << setw(8) << "Place" << setw(12) << "Statut"
         << "Remboursement\n"
         << string(68, '-') << "\n";
    for (const auto& r : agence.getReservations()) r->afficherLigne();
}

void afficherInfosReservation(const Agence& agence) {
    cout << "\n--- Informations d'une reservation ---\n";
    int id = lireEntier("Numero de la reservation : ", 1, 1000000);
    Reservation* r = agence.trouverReservation(id);
    if (!r)
        cout << "  [Refuse] Reservation introuvable.\n";
    else
        r->afficherDetails();
}

int main() {
    Agence agence("Agence Cotonou Voyages", "Cotonou");
    cout << "Bienvenue a l'" << agence.getNom() << " (" << agence.getVille()
         << ")\n";
    int choix = 0;
    do {
        afficherMenu();
        choix = lireEntier("Votre choix (1-10) : ", 1, 10);
        switch (choix) {
            case 1:  enregistrerPassager(agence);       break;
            case 2:  ajouterVol(agence);                break;
            case 3:  afficherVolsDisponibles(agence);   break;
            case 4:  rechercherVol(agence);             break;
            case 5:  effectuerReservation(agence);      break;
            case 6:  enregistrerPaiement(agence);       break;
            case 7:  annulerReservation(agence);        break;
            case 8:  consulterReservations(agence);     break;
            case 9:  afficherInfosReservation(agence);  break;
            case 10: cout << "Au revoir !\n";           break;
        }
    } while (choix != 10);
    return 0;
}
