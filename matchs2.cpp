#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <fstream>
using namespace std;

typedef vector<vector<int>> Matrice;

int nb_terrains;
int nb_creneaux;
int nb_equipes;
int nb_couples;
int nb_tabou;
int compteur(0);
int bb_cpt(0);
int bb_vois(1e6);
int lgL; // length left
int lgR; // length right
int TotVois(0);
int TotRec(0);
int nouv_recTot; // nouveau total de recouvrement
int nouv_voisTot; // nouveau total de voisinage
int objTot_b(1e6);
vector<int> PtRec; // points de recouvrement
vector<int> PtVois; // points de voisinage
vector<int> Equipe(4, 0);
vector<int> Vois(4, 0);
vector<int> Rec(4, 0);
vector<int> best_couples(2); // meilleur paire de couples à inverser
vector<int> b_nouvVois(4), b_nouvRec(4);

int PenVois(int n) { return n * (n - 1); } // penalité voisinage
int PenRec(int n) { return n * (n - 1); } // penalité recouvrement
int PenTotVois(int n) { return n * n * n; } // penalité total voisinage
int PenTotRec(int n) { return n * n * n; } // penalité total recouvrement

Matrice input();
void affiche(Matrice matrice);
void calcul_initial(); // calcul des blocs matrices et penalités
void takeon(int equipe, int creneau);
void takeoff(int equipe, int creneau); // échanger deux matchs
int gain_vois(int equipe, int cre_from, int cre_to);
int gain_rec(int equipe, int cre_from, int cre_to);
void best_2couples(); // trouve le meilleur échange de 2 couples
void invert();
bool not_tabou(int couple1, int couple2, int cre1, int cre2);
double Fct_Obj(int Vois, int Rec);
void Fct_Tabou(int& nb_tabou);
bool not_so_good(double objet);
void affiche_tout();

Matrice match(input());
Matrice Bloc_Cl; // matrice Bloc clash
Matrice Bloc_Lg; // matrice Bloc length
Matrice Bloc_Ps; // matrice Bloc position
Matrice tabou;
vector<int> bb_match(nb_couples);

int main()
{
    Fct_Tabou(nb_tabou);
    calcul_initial();
    affiche_tout();

    while (not_so_good(objTot_b)) {
        compteur++;
        best_2couples();
        invert();
        if ((compteur % 1000) == 0) {
            cout << "iteration no " << compteur << endl;
        }
    }

    for(int cp(0); cp < nb_couples; cp++) {
        match[cp][3] = bb_match[cp];
    }

    calcul_initial();
    affiche_tout();

    return 0;
}

Matrice input()
{    
    Matrice match;
    ifstream fin("C:\\TGI\\Mch2.dat");
    if (not fin.is_open()) {
        cout << "Erreur d'ouverture du fichier" << endl;
        return match;
    }

    fin >> nb_equipes >> nb_couples >> nb_terrains >> nb_creneaux;

    match.resize(nb_couples);

    for (int i(0); i < nb_couples; ++i) {
        match[i].resize(4);
    }

    for (int couple(0); couple < nb_couples; ++couple) {
        for (int i(0); i < 4; ++i) {
            fin >> match[couple][i];
        }
    }

    fin.close();

    return match;
}

void affiche(Matrice matrice)
{
    for (size_t i(0); i < matrice.size(); ++i) {
        for (size_t j(0); j < matrice[i].size(); ++j) {
            cout << matrice[i][j] << " ";
        }

        cout << endl;
    }
}

void calcul_initial()
{
    vector<int> temp0;
    vector<int> temp1;
    for (int i(0); i < nb_tabou; ++i) {
        temp0.push_back(-1);
        temp1.push_back(0);
    }
    tabou.push_back(temp0);
    tabou.push_back(temp1);
    
    vector<int> creneau_vide(nb_creneaux, 0);
    for (int equipe = 0; equipe < nb_equipes; ++equipe) {
        Bloc_Cl.push_back(creneau_vide);
        Bloc_Lg.push_back(creneau_vide);
        Bloc_Ps.push_back(creneau_vide);
    }

    for (const auto& aff : match) {
        int eq1 = aff[0];
        int eq2 = aff[1];
        int cre = aff[3];
        takeon(eq1, cre);
        takeon(eq2, cre);
    }

    vector<int> vide(nb_equipes, 0);

    for (int equipe(0); equipe < nb_equipes; ++equipe) {
        PtVois.push_back(0);
        PtRec.push_back(0);

        for (int cre = 0; cre < nb_creneaux; ++cre) {
            int leng = Bloc_Lg[equipe][cre];
            if (leng > 1) {
                PtVois[equipe] += PenVois(leng);
                cre += leng;
            }
        }

        for (int cre(0); cre < nb_creneaux; ++cre) {
            int rec = Bloc_Cl[equipe][cre];
            if (rec > 1) {
                PtRec[equipe] += PenRec(rec);
            }
        }

        TotVois += PenTotVois(PtVois[equipe]);
        TotRec += PenTotRec(PtRec[equipe]);
    }

    nouv_voisTot = TotVois;
    nouv_recTot = TotRec;
}

void takeon(int equipe, int creneau)
{
    int cal = ++Bloc_Cl[equipe][creneau];

    if (cal == 1) {
        if (creneau != 0) {
            lgL = Bloc_Lg[equipe][creneau - 1];
        } else {
            lgL = 0;
        }

        if (creneau != (nb_creneaux - 1)) {
            lgR = Bloc_Lg[equipe][creneau + 1];
        } else {
            lgR = 0;
        }

        int nombre = lgL + lgR + 1;

        for (int i = -lgL, j = 1; i <= lgR; ++i, ++j) {
            Bloc_Lg[equipe][creneau + i] = nombre;
            Bloc_Ps[equipe][creneau + i] = j;
        }
    }
}

void takeoff(int equipe, int creneau)
{
    int leng, bloc;
    
    int cal = --Bloc_Cl[equipe][creneau];
    if (cal == 0) {
        leng = Bloc_Lg[equipe][creneau];
        bloc = Bloc_Ps[equipe][creneau];
        Bloc_Lg[equipe][creneau] = 0;
        Bloc_Ps[equipe][creneau] = 0;

        int nombre = bloc - 1;

        if (nombre > 0) {
            for (int i(1); i <= nombre; ++i) {
                Bloc_Lg[equipe][creneau - i] = nombre;
                Bloc_Ps[equipe][creneau - i] = nombre - i + 1;
            }
        }

        nombre = leng - bloc;
        if (nombre > 0) {
            for (int i(1); i <= nombre; ++i) {
                Bloc_Lg[equipe][creneau + i] = nombre;
                Bloc_Ps[equipe][creneau + i] = i;
            }
        }
    }
}

int gain_vois(int equipe, int cre_from, int cre_to)
{
    int gain(0);
    int cal_from(Bloc_Cl[equipe][cre_from]);
    int cal_to(Bloc_Cl[equipe][cre_to]);
    int leng(Bloc_Lg[equipe][cre_from]);
    int bloc(Bloc_Ps[equipe][cre_from]);

    if (cre_to != 0) {
        lgL = Bloc_Lg[equipe][cre_to - 1];
    } else {
        lgL = 0;
    } // nombre de matchs avant cre_from
    if (cre_to != nb_creneaux - 1) {
        lgR = Bloc_Lg[equipe][cre_to + 1];
    } else {
        lgR = 0;
    } // nombre de matchs après cre_from

    if (cal_from == 1) {
        // cas 1 : cre_from < cre_to
        if (cre_to == cre_from + leng - bloc + 1) {
            gain = PenVois(bloc - 1) + PenVois(leng - bloc + 1 + lgR) - PenVois(leng) - PenVois(lgR);
            return gain;
        }
        // cas 2 : cre_to < cre_from
        if (cre_to == cre_from - bloc) {
            gain = PenVois(bloc + lgL) + PenVois(leng - bloc) - PenVois(leng) - PenVois(lgL);
            return gain;
        }

        // rien
        gain = PenVois(leng - bloc) + PenVois(bloc - 1) - PenVois(leng);
        return gain;
    } else if (cal_to == 0) {
        gain += PenVois(lgR + lgL + 1) - PenVois(lgL) - PenVois(lgR);
    }
    
    return gain;
}

int gain_rec(int equipe, int cre_from, int cre_to)
{
    int gain(0);
    int cal_from(Bloc_Cl[equipe][cre_from]);
    int cal_to(Bloc_Cl[equipe][cre_to]);

    if (cal_from >= 2) {
        gain = PenRec(cal_from - 1) - PenRec(cal_from);
    }
    if (cal_to) {
        gain += PenRec(cal_to + 1) - PenRec(cal_to);
    }

    return gain;
}

void best_2couples()
{
    int cre1, cre2;
    int voisTot_a(nouv_voisTot);
    int recTot_a(nouv_recTot);
    int VTot, RTot;
    int vieux;
    vector<int> nouv_vois(4), nouv_rec(4);

    for(int couple1(0); couple1 < nb_couples - 1; couple1++) {
        for (int couple2(couple1 + 1); couple2 < nb_couples; couple2++) {
            cre1 = match[couple1][3];
            cre2 = match[couple2][3];

            if (cre1 != cre2) {                
                Equipe[0] = match[couple1][0];
                Equipe[1] = match[couple1][1];
                Equipe[2] = match[couple2][0];
                Equipe[3] = match[couple2][1];

                if (Equipe[0] == Equipe[2]) {
                    Vois[0] = 0;
                    Vois[1] = gain_vois(Equipe[1], cre1, cre2);
                    Vois[2] = 0;
                    Vois[3] = gain_vois(Equipe[3], cre2, cre1);
                    Rec[0] = 0;
                    Rec[1] = gain_rec(Equipe[1], cre1, cre2);
                    Rec[2] = 0;
                    Rec[3] = gain_rec(Equipe[3], cre2, cre1);
                } else if (Equipe[1] == Equipe[2]) {
                    Vois[1] = 0;
                    Vois[0] = gain_vois(Equipe[0], cre1, cre2);
                    Vois[3] = 0;
                    Vois[2] = gain_vois(Equipe[3], cre2, cre1);
                    Rec[1] = 0;
                    Rec[0] = gain_rec(Equipe[0], cre1, cre2);
                    Rec[3] = 0;
                    Rec[2] = gain_rec(Equipe[3], cre2, cre1);
                } else if (Equipe[1] == Equipe[3]) {
                    Vois[1] = 0;
                    Vois[0] = gain_vois(Equipe[0], cre1, cre2);
                    Vois[3] = 0;
                    Vois[2] = gain_vois(Equipe[2], cre2, cre1);
                    Rec[1] = 0;
                    Rec[0] = gain_rec(Equipe[0], cre1, cre2);
                    Rec[3] = 0;
                    Rec[2] = gain_rec(Equipe[2], cre2, cre1);
                } else {
                    Vois[0] = gain_vois(Equipe[0], cre1, cre2);
                    Vois[1] = gain_vois(Equipe[1], cre1, cre2);
                    Vois[2] = gain_vois(Equipe[2], cre2, cre1);
                    Vois[3] = gain_vois(Equipe[3], cre2, cre1);
                    Rec[0] = gain_rec(Equipe[0], cre1, cre2);
                    Rec[1] = gain_rec(Equipe[1], cre1, cre2);
                    Rec[2] = gain_rec(Equipe[2], cre2, cre1);
                    Rec[3] = gain_rec(Equipe[3], cre2, cre1);
                }

                VTot = voisTot_a;
                RTot = recTot_a;

                for (int k(0); k < 4; k++) {                    
                    vieux = PtVois[Equipe[k]];
                    nouv_vois[k] = vieux + Vois[k];
                    VTot += PenTotVois(nouv_vois[k]) - PenTotVois(vieux);
                    
                    vieux = PtRec[Equipe[k]];
                    nouv_rec[k] = vieux + Rec[k];
                    RTot += PenTotRec(nouv_rec[k]) - PenTotRec(vieux);
                }

                double ObjTot(VTot + 60 * RTot * (compteur % (100 * nb_tabou)) / (100 * nb_tabou));

                if (ObjTot < objTot_b or not_tabou(couple1, couple2, cre1, cre2)) {
                    best_couples[0] = couple1;
                    best_couples[1] = couple2;

                    for (int k(0); k < 4; k++) {
                        b_nouvVois[k] = nouv_vois[k];
                        b_nouvRec[k] = nouv_rec[k];
                    }

                    nouv_voisTot = VTot;
                    nouv_recTot = RTot;
                    objTot_b = ObjTot;               
                }

                if (RTot == 0 and VTot < bb_vois) {
                    cout << "iteration : " << compteur << " voisinage : " << VTot;
                    bb_vois = VTot;
                    bb_cpt = compteur;
                    for (int couple(0); couple < nb_couples; couple++) {
                        bb_match[couple] = match[couple][3];
                    }

                    bb_match[couple1] = cre1;
                    bb_match[couple2] = cre2;
                }
            }
        }
    }

    if (objTot_b > 9.99e5) {
        cout << "Je suis bloque :(";
        return;
    }
}

void invert()
{
    int couple1, couple2, cre1, cre2, Pl_tab;

    couple1 = best_couples[0];
    couple2 = best_couples[1];
    cre1 = match[couple1][3];
    cre2 = match[couple2][3];
    Equipe[0] = match[couple1][0];
    Equipe[1] = match[couple1][1];
    Equipe[2] = match[couple2][0];
    Equipe[3] = match[couple2][1];

    if (Equipe[0] == Equipe[2]) {
        takeoff(Equipe[1], cre1);
        takeon(Equipe[1], cre2);
        PtVois[Equipe[1]] = b_nouvVois[1];
        PtRec[Equipe[1]] = b_nouvRec[1];
        takeoff(Equipe[3], cre2);
        takeon(Equipe[3], cre1);
        PtVois[Equipe[3]] = b_nouvVois[3];
        PtRec[Equipe[3]] = b_nouvRec[3];
    } else if (Equipe[1] == Equipe[2]) {
        takeoff(Equipe[0], cre1);
        takeon(Equipe[0], cre2);
        PtVois[Equipe[0]] = b_nouvVois[0];
        PtRec[Equipe[0]] = b_nouvRec[0];
        takeoff(Equipe[3], cre2);
        takeon(Equipe[3], cre1);
        PtVois[Equipe[3]] = b_nouvVois[3];
        PtRec[Equipe[3]] = b_nouvRec[3];
    } else if (Equipe[1] == Equipe[3]) {
        takeoff(Equipe[0], cre1);
        takeon(Equipe[0], cre2);
        PtVois[Equipe[0]] = b_nouvVois[0];
        PtRec[Equipe[0]] = b_nouvRec[0];
        takeoff(Equipe[2], cre2);
        takeon(Equipe[2], cre1);
        PtVois[Equipe[2]] = b_nouvVois[2];
        PtRec[Equipe[2]] = b_nouvRec[2];
    } else {
        takeoff(Equipe[0], cre1);
        takeon(Equipe[0], cre2);
        PtVois[Equipe[0]] = b_nouvVois[0];
        PtRec[Equipe[0]] = b_nouvRec[0];
        takeoff(Equipe[1], cre1);
        takeon(Equipe[1], cre2);
        PtVois[Equipe[1]] = b_nouvVois[1];
        PtRec[Equipe[1]] = b_nouvRec[1];
        takeoff(Equipe[2], cre2);
        takeon(Equipe[2], cre1);
        PtVois[Equipe[2]] = b_nouvVois[2];
        PtRec[Equipe[2]] = b_nouvRec[2];
        takeoff(Equipe[3], cre2);
        takeon(Equipe[3], cre1);
        PtVois[Equipe[3]] = b_nouvVois[3];
        PtRec[Equipe[3]] = b_nouvRec[3];
    }

    TotVois = nouv_voisTot;
    TotRec = nouv_recTot;

    if (TotRec == 0) {
        if (TotVois == bb_vois) {
            cout << "***";
        } else {
            cout << " !" << nouv_voisTot;
        }
    }

    match[couple1][3] = cre2;
    match[couple2][3] = cre1;

    Pl_tab = (2 * (compteur - 1)) % nb_tabou;
    vector<int> temp0(nb_tabou);
    vector<int> temp1(nb_tabou);
    temp0[Pl_tab] = couple1;
    temp1[Pl_tab] = cre1;
    temp0[Pl_tab + 1] = couple2;
    temp1[Pl_tab + 1] = cre2;

    tabou.push_back(temp0);
    tabou.push_back(temp1);
}

bool not_tabou(int couple1, int couple2, int cre1, int cre2)
{
    for (int tb(0); tb < nb_tabou; tb++) {
        if ((tabou[0][tb] == couple1 and tabou[1][tb] == cre1) or
            (tabou[0][tb] == couple2 and tabou[1][tb] == cre2)) {
            return false;
        }
    }

    return true;
}

double Fct_Obj(int Vois, int Rec)
{
    return (Vois + 60 * Rec * (compteur % (100 * nb_tabou)) / (100 * nb_tabou));
}

void Fct_Tabou(int& nb_tabou)
{
    nb_tabou = nb_creneaux * nb_couples / 5;
}

bool not_so_good(double objet)
{
    if (objet == 0 or (compteur - bb_cpt) > 4000) {
        return false;
    }
    return true;
}

void affiche_tout()
{
    cout << "iteration no " << compteur << endl << endl;

    cout << "tabou : " << endl;
    for (int tb(0); tb < nb_tabou; tb++) {
        cout << tabou[0][tb] << " " << tabou[1][tb] << " ! ";
    }

    cout << endl << "matchs" << endl;
    for (int cp(0); cp < nb_couples; cp++) {
        cout << match[cp][0] << " vs " << match[cp][1] << " - " << match[cp][2] << " : " << match[cp][3] << endl;
    }

    cout << endl << "blocs" << endl;
    for (int eq(0); eq < nb_equipes; eq++) {
        cout << "Equipe : " << eq << endl;
        for (int cre(0); cre < nb_creneaux; cre++) {
            cout << Bloc_Cl[eq][cre] << " " << Bloc_Lg[eq][cre] << " " << Bloc_Ps[eq][cre] << " ! ";
        }
        cout << endl;
    }

    cout << endl << "voisinage : " << endl;
    for (int eq(0); eq < nb_equipes; eq++) {
        cout << "equipe " << eq << " : " << PtVois[eq] << " ! ";
    }
    cout << endl << "total = " << TotVois << endl << endl;

    cout << "recouvrements : " << endl;
    for (int eq(0); eq < nb_equipes; eq++) {
        cout << "equipe " << eq << " : " << PtRec[eq] << " ! ";
    }
    cout << endl << "total = " << TotRec << endl << endl;

    cout << "resultat : " << endl;
    for (int cre(0); cre < nb_creneaux; cre++) {
        for (int cp(0); cp < nb_couples; cp++) {
            if (match[cp][3] == cre) {
                cout << match[cp][0] << " - " << match[cp][1] << " ! ";
            }
        }
        cout << endl;
    }
}
