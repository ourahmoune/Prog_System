#ifndef UTILS_H
#define UTILS_H
#define PRECISION 3
#define MON_FICHIER "utils.h"
#define PROJ_ID_CLIENT 5
#define PROJ_ID_MASTER 6
//TODO d'autres include éventuellement

#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
/******************************************
 * nombres aléatoires
 ******************************************/
// float aléatoire entre min et max (max non inclus : [min,max[), arrondi à <precision> chiffre(s) après la virgule
float ut_getAleaFloat(float min, float max, int precision);

// tableau de float aléatoires utilisant la fonction ci-dessus
float * ut_generateTab(int size, float min, float max, int precision);

//TODO d'autres fonctions utilitaires éventuellement
char * strFormat_of_float(const float f) ;
char * strFormat_of_int(const int x);
int my_semget_access(int id); 
int my_semget_create(int val, int id);
void entrerSC(int semId);
void sortirSC(int semId);
#endif
