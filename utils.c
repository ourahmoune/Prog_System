#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
//TODO d'autres include éventuellement

#include "utils.h"
#include "myassert.h"
#include <assert.h>
/******************************************
 * nombres aléatoires
 ******************************************/
static void ut_initAlea()
{
    static bool first = true;
    if (first)
    {
        srand(getpid());
        first = false;
    }
}

float ut_getAleaFloat(float min, float max, int precision)
{
    myassert(min < max, "min doit être strictement inférieur à max");
    myassert(precision >= 0, "la précision doit être positive");

    ut_initAlea();

    float r;
    int puiss = 1;
    for (int i = 0; i < precision; i++)
        puiss *= 10;

    do
    {
        int rInt = rand();
        r = ((float) rInt)/RAND_MAX * (max - min) + min;
        r = floor(r*puiss)/puiss;
    } while (r >= max);

    return r;
}

float * ut_generateTab(int size, float min, float max, int precision)
{
    float *t = malloc(size * sizeof(float));
    myassert(t != NULL, "allocation mémoire génération tableau float");
    for (int i = 0; i < size; i++)
        t[i] = ut_getAleaFloat(min, max, precision);

    // à mettre à true pour débuguer
    bool toPrint = false;
    if (toPrint)
    {
        printf("[");
        for (int i = 0; i < size; i++)
        {
            if (i != 0)
                printf(" ");
            printf("%g", t[i]);
        }
        printf("]\n");
    }
    return t;
}


//TODO d'autres fonctions utilitaires éventuellement
char * strFormat_of_float(const float f )
{
    char * res ;
    int nbChar = snprintf(NULL, 0,"%f",f);
    res = malloc((nbChar+1) * sizeof(char));
    sprintf(res,"%f",f);
    return res ; 
}
char * strFormat_of_int(const int x)
{
    char *res;
    int nbChar = snprintf(NULL, 0,"%d",x);
    res = malloc((nbChar+1) * sizeof(char));
    sprintf(res,"%d",x);
    return res;
}

int my_semget_create(int val, int id)
{
    key_t key;
    int semId;
    int ret;

    key = ftok(MON_FICHIER, id);
    assert(key != -1);
    semId = semget(key, 1, IPC_CREAT | IPC_EXCL | 0641);
    assert(semId != -1);
    ret = semctl(semId, 0, SETVAL, val);
    assert(ret != -1);

    return semId;
}
int my_semget_access(int id)
{
    key_t key;
    int semId;

    key = ftok(MON_FICHIER, id);
    assert(key != -1);

    semId = semget(key, 1, 0);
    assert(semId != -1);


    return semId;
}
void entrerSC(int semId)
{
    int ret;
    // paramètres : num sépamhore, opération, flags
    struct sembuf operationMoins = {0, -1, 0};
    ret = semop(semId, &operationMoins, 1);           // bloquant si sem == 0
    assert(ret != -1);
}
void sortirSC(int semId)
{
    int ret;
    // paramètres : num sépamhore, opération, flags
    struct sembuf operationPlus = {0, 1, 0};
    ret = semop(semId, &operationPlus, 1);
    assert(ret != -1);
}


