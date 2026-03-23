/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2026
 * Marc-André Gardner
 * 
 * Fichier principal
 ******************************************************************************/

#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "utils.h"
#include "emulateurClavier.h"
#include "tamponCirculaire.h"

// Les messages du createurRequetes ne depassent pas cette taille.
#define TAILLE_MAX_MESSAGE 4096


//============================================================================================================
// Section de test fait à l'aide de ChatGPT pour tester tamponCirculaire, et ce
// en ayant seulement cette section de code de fait, pour l'instant.
 
// METTRE A 1 POUR LANCER LES TESTS DU TAMPON CIRCULAIRE
// METTRE A 0 POUR LE FONCTIONNEMENT NORMAL DU PROGRAMME
// ============================================================
#define TEST_TAMPON_CIRCULAIRE 1
 
 
// ============================================================
// SECTION DE TESTS POUR LE TAMPON CIRCULAIRE
// ============================================================
#if TEST_TAMPON_CIRCULAIRE
 
static int testsReussis = 0;
static int testsEchoues = 0;
 
static void verifier(const char* nomTest, int condition){
    if(condition){
        printf("  [OK]     %s\n", nomTest);
        testsReussis++;
    } else {
        printf("  [ECHEC]  %s\n", nomTest);
        testsEchoues++;
    }
}
 
// Cree une requete de test. Le buffer data contient le texte SANS '\0' terminal,
// exactement comme le ferait le thread lecteur avec memcpy.
static struct requete creerRequeteTest(const char* texte){
    struct requete req;
    req.taille = strlen(texte);
    req.data = malloc(req.taille);
    memcpy(req.data, texte, req.taille);
    req.tempsReception = get_time();
    return req;
}
 
// Compare le contenu d'une requete avec un texte attendu (sans dependre de '\0')
static int requeteEgale(struct requete *req, const char* attendu){
    size_t lenAttendu = strlen(attendu);
    if (req->taille != lenAttendu) return 0;
    return memcmp(req->data, attendu, lenAttendu) == 0;
}
 
static void test1_insertion_simple(){
    printf("\n--- Test 1 : Insertion d'une seule requete ---\n");
    initTamponCirculaire(5);
    struct requete req = creerRequeteTest("hello");
    int ret = insererDonnee(&req);
    verifier("insererDonnee retourne 0", ret == 0);
    verifier("longueurFile vaut 1", longueurFile() == 1);
 
    // Consommer pour liberer
    struct requete sortie;
    consommerDonnee(&sortie);
    free(sortie.data);
}
 
static void test2_insertion_multiple(){
    printf("\n--- Test 2 : Insertion de plusieurs requetes ---\n");
    initTamponCirculaire(5);
    struct requete r1 = creerRequeteTest("aaa");
    struct requete r2 = creerRequeteTest("bbb");
    struct requete r3 = creerRequeteTest("ccc");
    insererDonnee(&r1);
    insererDonnee(&r2);
    insererDonnee(&r3);
    verifier("longueurFile vaut 3 apres 3 insertions", longueurFile() == 3);
 
    struct requete sortie;
    consommerDonnee(&sortie); free(sortie.data);
    consommerDonnee(&sortie); free(sortie.data);
    consommerDonnee(&sortie); free(sortie.data);
}
 
static void test3_consommation_ordre(){
    printf("\n--- Test 3 : Consommation dans le bon ordre (FIFO) avec memcmp ---\n");
    initTamponCirculaire(5);
    struct requete r1 = creerRequeteTest("premier");
    struct requete r2 = creerRequeteTest("deuxieme");
    struct requete r3 = creerRequeteTest("troisieme");
    insererDonnee(&r1);
    insererDonnee(&r2);
    insererDonnee(&r3);
 
    struct requete sortie;
    int ret = consommerDonnee(&sortie);
    verifier("consommerDonnee retourne 1", ret == 1);
    verifier("premiere sortie est 'premier'", requeteEgale(&sortie, "premier"));
    verifier("taille correcte (7)", sortie.taille == 7);
    verifier("longueurFile vaut 2 apres 1 consommation", longueurFile() == 2);
    free(sortie.data);
 
    consommerDonnee(&sortie);
    verifier("deuxieme sortie est 'deuxieme'", requeteEgale(&sortie, "deuxieme"));
    free(sortie.data);
 
    consommerDonnee(&sortie);
    verifier("troisieme sortie est 'troisieme'", requeteEgale(&sortie, "troisieme"));
    free(sortie.data);
}
 
static void test4_consommation_vide(){
    printf("\n--- Test 4 : Consommation sur tampon vide ---\n");
    initTamponCirculaire(5);
    struct requete sortie;
    int ret = consommerDonnee(&sortie);
    verifier("consommerDonnee retourne 0 quand vide", ret == 0);
    verifier("longueurFile vaut 0", longueurFile() == 0);
}
 
static void test5_consommation_jusqua_vide(){
    printf("\n--- Test 5 : Consommation jusqu'a vide ---\n");
    initTamponCirculaire(5);
    struct requete r1 = creerRequeteTest("un");
    struct requete r2 = creerRequeteTest("deux");
    insererDonnee(&r1);
    insererDonnee(&r2);
 
    struct requete sortie;
    consommerDonnee(&sortie); free(sortie.data);
    consommerDonnee(&sortie); free(sortie.data);
    verifier("longueurFile vaut 0 apres tout consommer", longueurFile() == 0);
 
    int ret = consommerDonnee(&sortie);
    verifier("consommerDonnee retourne 0 apres tout consommer", ret == 0);
}
 
static void test6_tampon_plein_ecrasement(){
    printf("\n--- Test 6 : Insertion quand tampon plein (ecrasement) ---\n");
    initTamponCirculaire(3);
    struct requete r1 = creerRequeteTest("AAA");
    struct requete r2 = creerRequeteTest("BBB");
    struct requete r3 = creerRequeteTest("CCC");
    struct requete r4 = creerRequeteTest("DDD");
    insererDonnee(&r1);
    insererDonnee(&r2);
    insererDonnee(&r3);
    verifier("longueurFile vaut 3 (plein)", longueurFile() == 3);
 
    // r4 ecrase r1 (AAA). Le free de AAA est fait par insererDonnee.
    insererDonnee(&r4);
    verifier("longueurFile reste 3 apres ecrasement", longueurFile() == 3);
 
    struct requete sortie;
    consommerDonnee(&sortie);
    verifier("la plus vieille est BBB (AAA ecrasee)", requeteEgale(&sortie, "BBB"));
    free(sortie.data);
 
    consommerDonnee(&sortie);
    verifier("ensuite CCC", requeteEgale(&sortie, "CCC"));
    free(sortie.data);
 
    consommerDonnee(&sortie);
    verifier("ensuite DDD (la nouvelle)", requeteEgale(&sortie, "DDD"));
    free(sortie.data);
 
    verifier("longueurFile vaut 0 apres tout lire", longueurFile() == 0);
}
 
static void test7_ecrasement_multiple(){
    printf("\n--- Test 7 : Ecrasements multiples ---\n");
    initTamponCirculaire(2);
    struct requete r1 = creerRequeteTest("A");
    struct requete r2 = creerRequeteTest("B");
    struct requete r3 = creerRequeteTest("C");
    struct requete r4 = creerRequeteTest("D");
    insererDonnee(&r1);
    insererDonnee(&r2);
    // r3 ecrase r1, r4 ecrase r2 (free faits par insererDonnee)
    insererDonnee(&r3);
    insererDonnee(&r4);
    verifier("longueurFile vaut 2", longueurFile() == 2);
 
    struct requete sortie;
    consommerDonnee(&sortie);
    verifier("premiere sortie est C (A et B ecrasees)", requeteEgale(&sortie, "C"));
    free(sortie.data);
 
    consommerDonnee(&sortie);
    verifier("deuxieme sortie est D", requeteEgale(&sortie, "D"));
    free(sortie.data);
}
 
static void test8_statistiques(){
    printf("\n--- Test 8 : Statistiques ---\n");
    initTamponCirculaire(5);
    resetStats();
 
    struct requete r1 = creerRequeteTest("stat1");
    struct requete r2 = creerRequeteTest("stat2");
    struct requete r3 = creerRequeteTest("stat3");
    insererDonnee(&r1);
    insererDonnee(&r2);
    insererDonnee(&r3);
 
    // Attendre un peu pour que le temps d'attente soit mesurable
    usleep(10000);  // 10ms
 
    struct requete sortie;
    consommerDonnee(&sortie); free(sortie.data);
    consommerDonnee(&sortie); free(sortie.data);
 
    struct statistiques stats;
    calculeStats(&stats);
 
    verifier("nombreRequetesEnAttente vaut 1", stats.nombreRequetesEnAttente == 1);
    verifier("nombreRequetesTraitees vaut 2", stats.nombreRequetesTraitees == 2);
    verifier("nombreRequetesPerdues vaut 0", stats.nombreRequetesPerdues == 0);
    verifier("tempsTraitementMoyen > 0", stats.tempsTraitementMoyen > 0.0);
    verifier("lambda > 0", stats.lambda > 0.0);
    verifier("mu > 0", stats.mu > 0.0);
    printf("  [INFO]   tempsTraitementMoyen = %f\n", stats.tempsTraitementMoyen);
    printf("  [INFO]   lambda = %f\n", stats.lambda);
    printf("  [INFO]   mu = %f\n", stats.mu);
    printf("  [INFO]   rho = %f\n", stats.rho);
 
    // Liberer la derniere requete restante
    consommerDonnee(&sortie); free(sortie.data);
}
 
static void test9_stats_requetes_perdues(){
    printf("\n--- Test 9 : Stats avec requetes perdues ---\n");
    initTamponCirculaire(2);
    resetStats();
 
    struct requete r1 = creerRequeteTest("x");
    struct requete r2 = creerRequeteTest("y");
    struct requete r3 = creerRequeteTest("z");
    insererDonnee(&r1);
    insererDonnee(&r2);
    // r3 ecrase r1
    insererDonnee(&r3);
 
    struct statistiques stats;
    calculeStats(&stats);
    verifier("nombreRequetesPerdues vaut 1", stats.nombreRequetesPerdues == 1);
    verifier("nombreRequetesEnAttente vaut 2", stats.nombreRequetesEnAttente == 2);
 
    struct requete sortie;
    consommerDonnee(&sortie); free(sortie.data);
    consommerDonnee(&sortie); free(sortie.data);
}
 
static void test10_reset_stats(){
    printf("\n--- Test 10 : Reset des statistiques ---\n");
    initTamponCirculaire(5);
    resetStats();
 
    // Generer de l'activite
    struct requete r1 = creerRequeteTest("abc");
    struct requete r2 = creerRequeteTest("def");
    insererDonnee(&r1);
    insererDonnee(&r2);
    usleep(5000);
    struct requete sortie;
    consommerDonnee(&sortie); free(sortie.data);
 
    // Verifier qu'il y a bien des stats non-nulles
    struct statistiques stats;
    calculeStats(&stats);
    verifier("avant reset: nombreRequetesTraitees vaut 1", stats.nombreRequetesTraitees == 1);
    verifier("avant reset: lambda > 0", stats.lambda > 0.0);
 
    // Reset
    resetStats();
 
    // Verifier que les compteurs sont a zero apres reset
    calculeStats(&stats);
    verifier("apres reset: nombreRequetesTraitees vaut 0", stats.nombreRequetesTraitees == 0);
    verifier("apres reset: nombreRequetesPerdues vaut 0", stats.nombreRequetesPerdues == 0);
    verifier("apres reset: lambda vaut 0", stats.lambda == 0.0);
    verifier("apres reset: mu vaut 0", stats.mu == 0.0);
    // nombreRequetesEnAttente ne se reset PAS : c'est un instantane du tampon
    verifier("apres reset: nombreRequetesEnAttente vaut 1 (toujours dans tampon)", stats.nombreRequetesEnAttente == 1);
 
    consommerDonnee(&sortie); free(sortie.data);
}
 
// ============================================================
// Test 11 : Concurrence minimale (producteur / consommateur)
// ============================================================
 
#define CONCURRENCE_NBR_REQUETES 100
 
static void* threadProducteur(void* arg){
    pthread_barrier_t* barriere = (pthread_barrier_t*)arg;
    pthread_barrier_wait(barriere);
 
    for (int i = 0; i < CONCURRENCE_NBR_REQUETES; i++){
        // Creer une requete avec un contenu unique
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "req%d", i);
 
        struct requete req;
        req.taille = len;
        req.data = malloc(len);
        memcpy(req.data, buf, len);
        req.tempsReception = get_time();
 
        insererDonnee(&req);
        // Petit delai pour simuler un rythme d'arrivee
        usleep(100);
    }
    return NULL;
}
 
static void* threadConsommateur(void* arg){
    pthread_barrier_t* barriere = (pthread_barrier_t*)arg;
    pthread_barrier_wait(barriere);
 
    int consommees = 0;
    int tentatives = 0;
    // On tente de consommer toutes les requetes, avec une limite de tentatives
    // pour ne pas boucler a l'infini si le producteur est plus lent
    while (consommees < CONCURRENCE_NBR_REQUETES && tentatives < CONCURRENCE_NBR_REQUETES * 20){
        struct requete req;
        int ret = consommerDonnee(&req);
        if (ret == 1){
            free(req.data);
            consommees++;
        } else {
            usleep(50);
        }
        tentatives++;
    }
 
    // Retourner le nombre de requetes consommees via un int* alloue
    int* resultat = malloc(sizeof(int));
    *resultat = consommees;
    return resultat;
}
 
static void test11_concurrence(){
    printf("\n--- Test 11 : Concurrence (producteur/consommateur) ---\n");
    // Tampon plus petit que le nombre de requetes pour forcer des situations
    // ou le tampon se remplit et ou il y a contention sur le mutex
    initTamponCirculaire(10);
    resetStats();
 
    pthread_barrier_t barriere;
    pthread_barrier_init(&barriere, NULL, 2);
 
    pthread_t prod, cons;
    pthread_create(&prod, NULL, threadProducteur, &barriere);
    pthread_create(&cons, NULL, threadConsommateur, &barriere);
 
    pthread_join(prod, NULL);
 
    void* retval;
    pthread_join(cons, &retval);
    int consommees = *((int*)retval);
    free(retval);
 
    unsigned int restantes = longueurFile();
 
    printf("  [INFO]   Requetes produites  : %d\n", CONCURRENCE_NBR_REQUETES);
    printf("  [INFO]   Requetes consommees : %d\n", consommees);
    printf("  [INFO]   Restantes en tampon : %u\n", restantes);
 
    // Le total consomme + restant + perdu doit egaliser le nombre produit
    struct statistiques stats;
    calculeStats(&stats);
    unsigned int total = consommees + restantes + stats.nombreRequetesPerdues;
    printf("  [INFO]   Requetes perdues    : %u\n", stats.nombreRequetesPerdues);
    printf("  [INFO]   Total (cons+rest+perdues) : %u\n", total);
 
    verifier("pas de crash", 1);  // Si on arrive ici, pas de crash
    verifier("longueurFile <= taille tampon (10)", restantes <= 10);
    verifier("total coherent (cons+rest+perdues == produites)", total == CONCURRENCE_NBR_REQUETES);
 
    // Vider le tampon
    struct requete sortie;
    while (consommerDonnee(&sortie) == 1){
        free(sortie.data);
    }
 
    pthread_barrier_destroy(&barriere);
}
 
// ============================================================
// Lancement de tous les tests
// ============================================================
 
static void lancerTousLesTests(){
    printf("========================================\n");
    printf("  TESTS DU TAMPON CIRCULAIRE\n");
    printf("========================================\n");
    test1_insertion_simple();
    test2_insertion_multiple();
    test3_consommation_ordre();
    test4_consommation_vide();
    test5_consommation_jusqua_vide();
    test6_tampon_plein_ecrasement();
    test7_ecrasement_multiple();
    test8_statistiques();
    test9_stats_requetes_perdues();
    test10_reset_stats();
    test11_concurrence();
    printf("\n========================================\n");
    printf("  RESULTATS : %d reussis, %d echoues\n", testsReussis, testsEchoues);
    printf("========================================\n");
}
 
#endif  // TEST_TAMPON_CIRCULAIRE
 
//============================================================================================================
 

static void* threadFonctionClavier(void* args){
    // Implementez ici votre fonction de thread pour l'ecriture sur le bus USB
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadClavier *infos = (struct infoThreadClavier *)args;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur

    // TODO

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Tenter d'obtenir une requete depuis le tampon circulaire avec consommerDonnee()
    // 2) S'il n'y en a pas, attendre un cours laps de temps (par exemple usleep(500))
    // 3) S'il y en a une, appeler ecrireCaracteres avec les informations requises
    // 4) Liberer la memoire du champ data de la requete avec la fonction free(), puisque
    //      la requete est maintenant terminee

    while(1){
       // TODO
    }
    return NULL;
}

// Fonction threadFonctionLecture() fait par Anthony Veillet
static void* threadFonctionLecture(void *args){

    // Implementez ici votre fonction de thread pour la lecture sur le named pipe
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadLecture *infos = (struct infoThreadLecture *)args;
    
    // Ces champs vous seront utiles pour l'appel a select()
    fd_set setFd;
    int nfds = infos->pipeFd + 1;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur

    // TODO --> Fait
    // Attendre que le thread clavier soit aussi pret
    pthread_barrier_wait(infos->barriere);

    // Buffer pour accumuler le message en cours de reception
    char messageBuffer[TAILLE_MAX_MESSAGE];
    int messagePos = 0;
 
    // Petit buffer pour chaque appel a read()
    char readBuffer[256];

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Remplir setFd en utilisant FD_ZERO et FD_SET correctement, pour faire en sorte
    //      d'attendre sur infos->pipeFd
    // 2) Appeler select(), sans timeout, avec setFd comme argument de lecture (on veut bien
    //      lire sur le pipe)
    // 3) Lire les valeurs sur le named pipe
    // 4) Si une de ses valeurs est le caracteres ASCII EOT (0x4), alors c'est la fin d'un
    //      message. Vous creez alors une nouvelle requete et utilisez insererDonnee() pour
    //      l'inserer dans le tampon circulaire. Notez que le caractere EOT ne doit PAS se
    //      retrouver dans le champ data de la requete! N'oubliez pas egalement de donner
    //      la bonne valeur aux champs taille et tempsReception.

    while(1){
        // TODO ---> Fait
        FD_ZERO(&setFd);
        FD_SET(infos->pipeFd, &setFd);

        // Call select() sans timeout (bloque jusqu'a ce que des donnees arrivent)
        int selectRet = select(nfds, &setFd, NULL, NULL, NULL);
        if (selectRet <= 0) {
            // Erreur ou interruption par un signal, on recommence
            continue;
        }
 
        // Lire donnees dispo sur le pipe
        ssize_t nbLus = read(infos->pipeFd, readBuffer, sizeof(readBuffer));
        if (nbLus <= 0) {
            // Pipe ferme ou erreur de lecture
            continue;
        }
 
        // 4) Parcourir chaque octet lu
        for (int i = 0; i < nbLus; i++) {
 
            if (readBuffer[i] == 0x04) {
                // Soit caractere EOT, fin du message
 
                // Creer new requete
                struct requete req;
                req.tempsReception = get_time();
                req.taille = messagePos;
 
                // Allouer et copier les donnees du message
                req.data = malloc(messagePos);
                if (req.data == NULL) {
                    fprintf(stderr, "Erreur malloc dans threadFonctionLecture\n");
                    messagePos = 0;
                    continue;
                }

                // Verifier avec de faire memcpy
                if (req.data == NULL) {
                    fprintf(stderr, "Pointeur NULL avant memcpy dans threadFonctionLecture\n");
                }
                memcpy(req.data, messageBuffer, messagePos);
 
                // Insert requete dans le tampon circulaire
                insererDonnee(&req);
 
                // Reset buffer pour le prochain message
                messagePos = 0;
 
            } else {
                // Caractere normal, donc à accumuler
                if (messagePos < TAILLE_MAX_MESSAGE) {
                    messageBuffer[messagePos] = readBuffer[i];
                    messagePos++;
                }
            }
        }
    }

    return NULL;
}

int main(int argc, char* argv[]){

#if TEST_TAMPON_CIRCULAIRE
    // Mode test : on lance les tests et on quitte
    lancerTousLesTests();
    return 0;
#endif

    if(argc < 4){
        printf("Pas assez d'arguments! Attendu : ./emulateurClavier cheminPipe tempsAttenteParPaquet tailleTamponCirculaire\n");
    }

    // A ce stade, vous pouvez consider que:
    // argv[1] contient un chemin valide vers un named pipe
    // argv[2] contient un entier valide (que vous pouvez convertir avec atoi()) representant le nombre de microsecondes a
    //      attendre a chaque envoi de paquet
    // argv[3] contient un entier valide (que vous pouvez convertir avec atoi()) contenant la taille voulue pour le tampon
    //      circulaire

    // Vous avez plusieurs taches d'initialisation a faire :
    //
    // 1) Ouvrir le named pipe

    // TODO

    // 2) Declarer et initialiser la barriere
    
    // TODO

    // 3) Initialiser le tampon circulaire avec la bonne taille

    // TODO

    // 4) Creer et lancer les threads clavier et lecteur, en leur passant les bons arguments dans leur struct de configuration respective
    
    // TODO


    // La boucle de traitement est deja implementee pour vous. Toutefois, si vous voulez eviter l'affichage des statistiques
    // (qui efface le terminal a chaque fois), vous pouvez commenter la ligne afficherStats().
    struct statistiques stats;
    double tempsDebut = get_time();
    while(1){
        // Affichage des statistiques toutes les 2 secondes
        calculeStats(&stats);
        afficherStats((unsigned int)(round(get_time() - tempsDebut)), &stats);
        resetStats();
        usleep(2e6);
    }
    return 0;
}
