/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2026
 * Marc-André Gardner
 * 
 * Finalisé par Anthony Veillet, session H26
 * 
 * Fichier implémentant les fonctions de gestion du tampon circulaire
 ******************************************************************************/

#include "tamponCirculaire.h"
#include <pthread.h>

// Plusieurs variables globales statiques (pour qu'elles ne soient accessible que dans les
// fonctions de ce fichier) sont declarees ici. Elle servent a conserver l'etat du tampon
// circulaire ainsi qu'a mesurer certains elements utiles au calcul des statistiques.
// Vous etes libres d'en creer d'autres si vous en voyez le besoin.

// Pointe vers la memoire allouee pour le tampon circulaire
static char* memoire;

// Taile du tampon circulaire (en nombre d'elements de type struct requete)
static size_t memoireTaille;

// Positions de lecture et d'ecriture, et longueur actuelle du tampon circulaire
static unsigned int posLecture, posEcriture, longueurCourante;

// Mutex permettant de proteger les acces au tampon circulaire
// N'oubliez pas que _deux_ threads vont tenter de faire des operations en parallele!
pthread_mutex_t mutexTampon;

// Pour les statistiques
static unsigned int nombreRequetesRecues, nombreRequetesTraitees, nombreRequetesPerdues;

// On calcule les statistiques par période de N secondes.
// Le tempsDebutPeriode permet de se rappeler du temps de debut de la periode où les statistiques sont mesurées
// La variable sommeTempsAttente contient la somme de toutes les periodes d'attente pour les requetes
// (vous pourrez donc calculer la moyenne du temps d'attente en utilisant les autres variables sur le
// nombre de requetes).
static double tempsDebutPeriode, sommeTempsAttente, sommeTempsService;

int initTamponCirculaire(size_t taille){
    // Initialisez ici:
    // La memoire, en utilisant malloc ou calloc (rappelez-vous que votre tampon circulaire doit
    // pouvoir contenir _taille_ fois la taille d'une struct requete)
    //
    // Les positions de lecture, d'ecriture et de longueur courante.
    //
    // Le mutex
    //
    // Les variables de statistiques

    // 1) Init mémoire
    memoire = calloc(taille, sizeof(struct requete));
    if (memoire == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire du tampon circulaire\n");
        return -1;
    }
    memoireTaille = taille;

    // 2) Init posLecture
    posLecture = 0;

    // 3) Init posEcriture
    posEcriture = 0;

    // 4) Init longueurCourante
    longueurCourante = 0;

    // 5) Init mutex
    if (pthread_mutex_init(&mutexTampon, NULL) != 0){
        fprintf(stderr, "Erreur lors de la création du mutex\n");
        return -1;
    }

    // 6) Init variables de stats
    nombreRequetesRecues = 0;
    nombreRequetesTraitees = 0;
    nombreRequetesPerdues = 0;
    tempsDebutPeriode = get_time();
    sommeTempsAttente = 0;
    sommeTempsService = 0;


    return 0;
}

void resetStats(){
    // Reinitialise les variables de statistique
    nombreRequetesRecues = 0;
    nombreRequetesTraitees = 0;
    nombreRequetesPerdues = 0;
    tempsDebutPeriode = get_time();
    sommeTempsAttente = 0;
    sommeTempsService = 0;

}

void calculeStats(struct statistiques *stats){
    double duree = get_time() - tempsDebutPeriode;

    stats->nombreRequetesEnAttente = longueurCourante;
    stats->nombreRequetesTraitees = nombreRequetesTraitees;
    stats->nombreRequetesPerdues = nombreRequetesPerdues;

    if (nombreRequetesTraitees > 0)
        stats->tempsTraitementMoyen = sommeTempsAttente / nombreRequetesTraitees;
    else
        stats->tempsTraitementMoyen = 0.0;

    if (duree > 0) {
        stats->lambda = (double)nombreRequetesRecues / duree;
    } else {
        stats->lambda = 0.0;
    }

    // mu = capacite de service = 1 / tempsServiceMoyen
    if (sommeTempsService > 0 && nombreRequetesTraitees > 0) {
        stats->mu = (double)nombreRequetesTraitees / sommeTempsService;
    } else {
        stats->mu = 0.0;
    }

    if (stats->mu > 0)
        stats->rho = stats->lambda / stats->mu;
    else
        stats->rho = 0.0;
}

int insererDonnee(struct requete *req){
    // Dans cette fonction, vous devez :
    //
    // Determiner a quel endroit copier la requete req dans le tampon circulaire
    //
    // Copier celle-ci
    //
    // Mettre a jour posEcriture et longueurCourante (toujours) et possiblement
    // posLecture (si vous vous etes "mordu la queue" et que vous etes revenu au
    // debut de votre tampon circulaire, il faut aussi repousser le pointeur de lecture
    // pour que le prochain element lu soit le plus ancien!)
    //
    // Mettre a jour les variables necessaires aux statistiques (comme nombreRequetesRecues, par exemple)
    //
    // N'oubliez pas de proteger les operations qui le necessitent par un mutex!
   
    // Mutex pour section critique
    if (pthread_mutex_lock(&mutexTampon) != 0){
        fprintf(stderr, "Erreur lors du verrouillage du mutex\n");
        return -1;
    }

    struct requete* tampon = (struct requete*)memoire;

    // Si le tampon est plein, on écrase la plus vieille requête
    if (longueurCourante == memoireTaille) {
        // Libérer la mémoire de la requête qu'on va écraser
        free(tampon[posEcriture].data);
        // Pousser posLecture car on écrase la plus vieille
        posLecture = (posLecture + 1) % memoireTaille;
        nombreRequetesPerdues++;
    }
    else {
        longueurCourante++;
    }

    // Copier la requête dans le tampon
    tampon[posEcriture] = *req;

    // Avancer posEcriture
    posEcriture = (posEcriture + 1) % memoireTaille;

    nombreRequetesRecues++;

    if (pthread_mutex_unlock(&mutexTampon) != 0){
        fprintf(stderr, "Erreur lors du déverrouillage du mutex\n");
        return -1;
    }

    return 0;
}

int consommerDonnee(struct requete *req){
    // Dans cette fonction, vous devez :
    //
    // Determiner si une requete est disponible dans le tampon circulaire
    //
    // S'il n'y en a _pas_, retourner 0.
    //
    // S'il y en a une, alors :
    //      Copier cette requete dans la structure passee en argument
    //      Modifier la valeur de posLecture et longueurCourante
    //      Mettre a jour les variables necessaires aux statistiques (comme sommeTempsAttente)
    //      Retourner 1 pour indiquer qu'une requete disponible a ete copiee dans req.
    //
    // N'oubliez pas de proteger les operations qui le necessitent par un mutex!
    
    // Mutex pour section critique
    if (pthread_mutex_lock(&mutexTampon) != 0){
        fprintf(stderr, "Erreur lors du verrouillage du mutex\n");
        return -1;
    }

    // Vérifier si aucune req dispo
    if (longueurCourante == 0){
        if (pthread_mutex_unlock(&mutexTampon) != 0){
            fprintf(stderr, "Erreur lors du déverrouillage du mutex\n");
            return -1;
        }
        return 0;
    }


    struct requete* tampon = (struct requete*)memoire;

    // Copier la requête vers l'espace de l'appelant
    *req = tampon[posLecture];

    // Avancer posLecture
    posLecture = (posLecture + 1) % memoireTaille;
    longueurCourante--;

    nombreRequetesTraitees++;
    sommeTempsAttente += get_time() - req->tempsReception;

    if (pthread_mutex_unlock(&mutexTampon) != 0){
        fprintf(stderr, "Erreur lors du déverrouillage du mutex\n");
        return -1;
    }

    return 1;

}

unsigned int longueurFile(){
    // Retourne la longueur courante de la file contenue dans votre tampon circulaire.ee
    return longueurCourante;
}

void ajouterTempsService(double temps){
    pthread_mutex_lock(&mutexTampon);
    sommeTempsService += temps;
    pthread_mutex_unlock(&mutexTampon);
}