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
 
static struct requete creerRequeteTest(const char* texte){
    struct requete req;
    req.taille = strlen(texte);
    req.data = malloc(req.taille + 1);
    strcpy(req.data, texte);
    req.tempsReception = get_time();
    return req;
}
 
static void test1_insertion_simple(){
    printf("\n--- Test 1 : Insertion d'une seule requete ---\n");
    initTamponCirculaire(5);
    struct requete req = creerRequeteTest("hello");
    int ret = insererDonnee(&req);
    verifier("insererDonnee retourne 0", ret == 0);
    verifier("longueurFile vaut 1", longueurFile() == 1);
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
}
 
static void test3_consommation_ordre(){
    printf("\n--- Test 3 : Consommation dans le bon ordre (FIFO) ---\n");
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
    verifier("premiere sortie est 'premier'", strcmp(sortie.data, "premier") == 0);
    verifier("taille correcte", sortie.taille == strlen("premier"));
    verifier("longueurFile vaut 2 apres 1 consommation", longueurFile() == 2);
    consommerDonnee(&sortie);
    verifier("deuxieme sortie est 'deuxieme'", strcmp(sortie.data, "deuxieme") == 0);
    consommerDonnee(&sortie);
    verifier("troisieme sortie est 'troisieme'", strcmp(sortie.data, "troisieme") == 0);
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
    consommerDonnee(&sortie);
    consommerDonnee(&sortie);
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
    insererDonnee(&r4);
    verifier("longueurFile reste 3 apres ecrasement", longueurFile() == 3);
    struct requete sortie;
    consommerDonnee(&sortie);
    verifier("la plus vieille est BBB (AAA ecrasee)", strcmp(sortie.data, "BBB") == 0);
    consommerDonnee(&sortie);
    verifier("ensuite CCC", strcmp(sortie.data, "CCC") == 0);
    consommerDonnee(&sortie);
    verifier("ensuite DDD (la nouvelle)", strcmp(sortie.data, "DDD") == 0);
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
    insererDonnee(&r3);
    insererDonnee(&r4);
    verifier("longueurFile vaut 2", longueurFile() == 2);
    struct requete sortie;
    consommerDonnee(&sortie);
    verifier("premiere sortie est C (A et B ecrasees)", strcmp(sortie.data, "C") == 0);
    consommerDonnee(&sortie);
    verifier("deuxieme sortie est D", strcmp(sortie.data, "D") == 0);
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
    usleep(10000);
    struct requete sortie;
    consommerDonnee(&sortie);
    consommerDonnee(&sortie);
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
    insererDonnee(&r3);
    struct statistiques stats;
    calculeStats(&stats);
    verifier("nombreRequetesPerdues vaut 1", stats.nombreRequetesPerdues == 1);
    verifier("nombreRequetesEnAttente vaut 2", stats.nombreRequetesEnAttente == 2);
}
 
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
                if (req.data == NULL || messageBuffer == NULL) {
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
