#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>  /* exit */
#include <sys/wait.h> 	/* wait */
#include <string.h>   /* bibliothèque string */
#include <stdbool.h>
#include <fcntl.h>    /* opérations sur les fichiers */
#include "readcmd.h"
#include"liste.h"

pListe liste;
pid_t pid_fils = 0;
bool foreground;
/** La handler du signal SIGTSTP */
void handler_STOP(int signal_num) {
    if (pid_fils != 0) {
        if (foreground == true) { /** on envoir le signal SIGSTOP pour suspendre juste au fils en avant-plan */
            kill(pid_fils,SIGSTOP); 
            printf("[%d]    Arrêté \n", indice(&liste,pid_fils));
        }
    } else { /** S'il y'a pas de commande en avant-plan on affiche un message d'erreur */
        perror("Aucune commande n'est en cours \n");
    }
}

/** La handler du signal SIGTSTP */
void handler_INT(int signal_num) {
    if (pid_fils != 0) {
        if(foreground == true) { /** on envoir le signal SIGKILL pour tuer juste au fils en avant-plan */
            kill(pid_fils,SIGKILL);
            printf("\n");
        }
    } else { /** S'il y'a pas de commande en avant-plan on affiche un message d'erreur */
        perror("Aucune commande n'est en cours \n");
    }
}
 
/** La commande interne stop jobs 
parametres :
IN : idprocessus : l'identifiant du processus qui est propre au minishell
*/
void sj (int idprocessus) {

    if (existe((&liste), idprocessus)) { /** Si l'identifiant existe on cherche son pid sur la liste des jobs et on le suspend par l'envoie de SIGSTOP */
        
        int indice_pro = indice_id(&liste, idprocessus);
        
        int PID = (&liste)->Liste[indice_pro].pid;
        
        kill(PID,SIGSTOP);
        
    } else { /** Si l'identifiant n'existe pas on affiche un message d'erreur */
        printf("Vous devez entrer un identifiant valide \n"); 
    }

}

/** La commande interne background
parametres :
IN : idprocessus : l'identifiant du processus qui est propre au minishell
*/
void bg(int idprocessus) {

    if (existe((&liste), idprocessus)) { /** Si l'identifiant existe on cherche son pid sur la liste des jobs et on le reprend par l'envoie de SIGCONT et on insiste sur le fait que foreground est false */
        
        int indice_pro = indice_id(&liste, idprocessus);
        
        int PID = (&liste)->Liste[indice_pro].pid;
        
        kill(PID, SIGCONT);
        
        foreground = false;
        
    } else { /** Si l'identifiant n'existe pas on affiche un message d'erreur */
        printf("Vous devez entrer un identifiant valide \n"); 
    }

}

/** La commande interne foreground
parametres :
IN : idprocessus : l'identifiant du processus qui est propre au minishell
*/
void fg(int idprocessus) {

    if (existe((&liste), idprocessus)) { /** Si l'identifiant existe on cherche son pid sur la liste des jobs et on le reprend par l'envoie de SIGCONT (s'il est suspendu il va se reprendre sinon rien ne va passer) et on attribue a foreground la valeur true pour qu'il s'exécute en avant-plan*/
        
        int indice_pro = indice_id(&liste, idprocessus);
        
        int PID = (&liste)->Liste[indice_pro].pid;
        
        kill(PID, SIGCONT);
        
        foreground = true;
        
    } else {
        printf("Vous devez entrer un identifiant valide \n"); 
    }

}

/** La commande interne cd
parametres :
IN : commandes = la ligne de commande
IN : repertoire_actuelle = le répertoire actuelle
*/
void cd(struct cmdline* commandes, char* repertoire_actuelle) {

    char *nouveau_repertoire;

    if((commandes->seq)[0][1]==NULL || strcmp((commandes->seq)[0][1], "~")==0) { /** Si la commande est de la forme "cd" ou "cd ~" on se retrove au dossier personnel */
        nouveau_repertoire = "/home/";
        char buffer1[strlen(repertoire_actuelle)];
        char buffer2[strlen(repertoire_actuelle)];
        
        char *p = strtok(strcpy(buffer2, repertoire_actuelle), "/");
        p = strtok(NULL, "/");
        
        strcat(strcpy(buffer1, nouveau_repertoire), p);
        
        chdir(buffer1);
        
    } else {
        if (chdir(commandes->seq[0][1]) == -1) { /** Sinon on va nous retrouver dans l'endroit indiqué avec un perror si y a erreur (chemin inexistant par exemple) */ 
            perror("Répertoire invalide");
            
        }
    }

}




int main(int argc, char* argv[]) {
    Initialiser(&liste);  /** initialiser liste jobs */
    struct cmdline* commandes;
    char *commande, *repertoire_actuelle;
    
    /* Clear the screen */
    system("clear");
    
    /** Le boolean qui va servir comme paramètre de sortie du boucle */
    bool sortie = false;

    do {
    
        /** L'attente si le fils est en avant-plan */
        while (foreground != false) {
            pause();
        }
    
        /** L'affichage du répertoire actuel */
        repertoire_actuelle = getcwd(NULL, 0);
        printf("%s >> ", repertoire_actuelle);
        
        /** La lecture de la ligne de commande */
        commandes = readcmd();

        /** on reste au minishell si on tappe entrer sans rien écrire */
        if (commandes->seq[0] != NULL) {
        
            commande = commandes -> seq[0][0];
            
            /* Sortir quand exit est saisie au clavier */
            if (strcmp(commande, "exit") == 0) {
                sortie = true;
            }
            
            /* Le traitement interne de cd */
            else if (strcmp(commande, "cd") == 0) {
                cd(commandes, repertoire_actuelle);
            } 
            /** La commande interne liste jobs */
            else if (strcmp(commande, "lj") == 0) {
                Afficher(&liste);
            } 
            /** La commande interne stop jobs */
            else if (strcmp(commande, "sj") == 0) {
                sj(atoi((commandes->seq)[0][1]));
            } 
            /** La commande interne background */
            else if (strcmp(commande, "bg") == 0) {
                bg(atoi((commandes->seq)[0][1]));
            }
            /** La commande interne foreground */
            else if (strcmp(commande, "fg") == 0) {
                fg(atoi((commandes->seq)[0][1]));
            } 
            /** La commande interne suspendre */
            else if (strcmp(commande, "susp") == 0) {
                kill(getpid(),SIGSTOP);
            } 
            /** Traitement des commandes externes */
           else {

                /** On crée un fils par fork */
                pid_fils = fork();
            
                if (pid_fils == -1) { /** Si la céation échoue */
                    printf("Erreur fork");
                    exit(1);
                
                } else if (pid_fils == 0) { /** fils*/
                    execvp(commande, commandes -> seq[0]);
                    perror("execvp");
                    exit(1);
                    
                } else { /** père */
                
                    int codeTerm;
                    waitpid(pid_fils, &codeTerm, 0);
                }
                
            }
        }
    } while (!(sortie));

    return EXIT_SUCCESS;
}
