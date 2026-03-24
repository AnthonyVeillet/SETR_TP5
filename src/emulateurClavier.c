/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2026
 * Marc-André Gardner
 * 
 * Fichier implémentant les fonctions de l'emulateur de clavier
 ******************************************************************************/

#include "emulateurClavier.h"

FILE* initClavier(){
    // Deja implementee pour vous
    FILE* f = fopen(FICHIER_CLAVIER_VIRTUEL, "wb");
    if (f != NULL){
        setbuf(f, NULL);        // On desactive le buffering pour eviter tout delai
    }
    return f;
}


int ecrireCaracteres(FILE* periphClavier, const char* caracteres, size_t len, unsigned int tempsTraitementParPaquetMicroSecondes){
    if (periphClavier == NULL || caracteres == NULL) return -1;

    unsigned char packet[LONGUEUR_USB_PAQUET];
    unsigned char release[LONGUEUR_USB_PAQUET];
    memset(packet, 0, sizeof(packet));
    memset(release, 0, sizeof(release));

    size_t writtenChars = 0;

    // packet layout: [0]=modifier, [1]=0, [2..7]=keycodes (up to 6)
    int nextIndex = 2; // next position in packet to fill
    int currentModifier = 0; // 0 or 2 (Left Shift)

    for (size_t i = 0; i < len; i++){
        unsigned char ch = (unsigned char)caracteres[i];
        int code = 0;
        int mod = 0;

        if (ch >= 'a' && ch <= 'z'){
            code = 4 + (ch - 'a');
            mod = 0;
        } else if (ch >= 'A' && ch <= 'Z'){
            code = 4 + (ch - 'A');
            mod = 2; // Left Shift
        } else if (ch >= '1' && ch <= '9'){
            code = 30 + (ch - '1');
            mod = 0;
        } else if (ch == '0'){
            code = 39;
            mod = 0;
        } else if (ch == ' '){
            code = 44;
            mod = 0;
        } else if (ch == ','){
            code = 54;
            mod = 0;
        } else if (ch == '.'){
            code = 55;
            mod = 0;
        } else if (ch == '\n'){
            code = 40; // ENTER
            mod = 0;
        } else {
            // caractère non supporté → ignorer
            continue;
        }

        // If packet is empty, set current modifier
        if (nextIndex == 2){
            currentModifier = mod;
        }

        // If modifier differs and packet already has keys, flush it first
        if (mod != currentModifier && nextIndex > 2){
            packet[0] = (unsigned char)currentModifier;
            if (fwrite(packet, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
            if (fwrite(release, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
            usleep(tempsTraitementParPaquetMicroSecondes);
            memset(packet, 0, sizeof(packet));
            nextIndex = 2;
            currentModifier = mod;
        }

        // If packet full (6 keys), flush before adding
        if (nextIndex > 7){
            packet[0] = (unsigned char)currentModifier;
            if (fwrite(packet, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
            if (fwrite(release, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
            usleep(tempsTraitementParPaquetMicroSecondes);
            memset(packet, 0, sizeof(packet));
            nextIndex = 2;
            currentModifier = mod;
        }

        // Place code into packet
        packet[nextIndex++] = (unsigned char)code;
        writtenChars++;
    }

    // Flush remaining keys in packet if any
    if (nextIndex > 2){
        packet[0] = (unsigned char)currentModifier;
        if (fwrite(packet, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
        if (fwrite(release, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) return -1;
        usleep(tempsTraitementParPaquetMicroSecondes);
    }

    return (int)writtenChars;
}
