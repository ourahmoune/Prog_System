#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"
#include "myassert.h"
#include "config.h"

#include "client_master.h"

#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/************************************************************************
 * chaines possibles pour le premier paramètre de la ligne de commande
 ************************************************************************/
#define TK_STOP        "stop"             // arrêter le master
#define TK_HOW_MANY    "howmany"          // combien d'éléments dans l'ensemble
#define TK_MINIMUM     "min"              // valeur minimale de l'ensemble
#define TK_MAXIMUM     "max"              // valeur maximale de l'ensemble
#define TK_EXIST       "exist"            // test d'existence d'un élément, et nombre d'exemplaires
#define TK_SUM         "sum"              // somme de tous les éléments
#define TK_INSERT      "insert"           // insertion d'un élément
#define TK_INSERT_MANY "insertmany"       // insertions de plusieurs éléments aléatoires
#define TK_PRINT       "print"            // debug : demande aux master/workers d'afficher les éléments
#define TK_LOCAL       "local"            // lancer un calcul local (sans master) en multi-thread


/************************************************************************
 * structure stockant les paramètres du client
 * - les infos pour communiquer avec le master
 * - les infos pour effectuer le travail (cf. ligne de commande)
 *   (note : une union permettrait d'optimiser la place mémoire)
 ************************************************************************/
typedef struct {
    // communication avec le master
    //TODO
    int fd_master_client ;
    int fd_client_master;
    // infos pour le travail à faire (récupérées sur la ligne de commande)
    int order;     // ordre de l'utilisateur (cf. CM_ORDER_* dans client_master.h)
    float elt;     // pour CM_ORDER_EXIST, CM_ORDER_INSERT, CM_ORDER_LOCAL
    int nb;        // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float min;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float max;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    int nbThreads; // pour CM_ORDER_LOCAL
} Data;


/************************************************************************
 * Usage
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usages : %s <ordre> [[[<param1>] [<param2>] ...]]\n", exeName);
    fprintf(stderr, "   $ %s " TK_STOP "\n", exeName);
    fprintf(stderr, "          arrêt master\n");
    fprintf(stderr, "   $ %s " TK_HOW_MANY "\n", exeName);
    fprintf(stderr, "          combien d'éléments dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MINIMUM "\n", exeName);
    fprintf(stderr, "          plus petite valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MAXIMUM "\n", exeName);
    fprintf(stderr, "          plus grande valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_EXIST " <elt>\n", exeName);
    fprintf(stderr, "          l'élement <elt> est-il présent dans l'ensemble ?\n");
    fprintf(stderr, "   $ %s " TK_SUM "\n", exeName);
    fprintf(stderr, "           somme des éléments de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT " <elt>\n", exeName);
    fprintf(stderr, "          ajout de l'élement <elt> dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT_MANY " <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          ajout de <nb> élements (dans [<min>,<max>[) aléatoires dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_PRINT "\n", exeName);
    fprintf(stderr, "          affichage trié (dans la console du master)\n");
    fprintf(stderr, "   $ %s " TK_LOCAL " <nbThreads> <elt> <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          combien d'exemplaires de <elt> dans <nb> éléments (dans [<min>,<max>[)\n"
                    "          aléatoires avec <nbThreads> threads\n");

    if (message != NULL)
        fprintf(stderr, "message :\n    %s\n", message);

    exit(EXIT_FAILURE);
}


/************************************************************************
 * Analyse des arguments passés en ligne de commande
 ************************************************************************/
static void parseArgs(int argc, char * argv[], Data *data)
{
    data->order = CM_ORDER_NONE;

    if (argc == 1)
        usage(argv[0], "Il faut préciser une commande");

    // première vérification : la commande est-elle correcte ?
    if (strcmp(argv[1], TK_STOP) == 0)
        data->order = CM_ORDER_STOP;
    else if (strcmp(argv[1], TK_HOW_MANY) == 0)
        data->order = CM_ORDER_HOW_MANY;
    else if (strcmp(argv[1], TK_MINIMUM) == 0)
        data->order = CM_ORDER_MINIMUM;
    else if (strcmp(argv[1], TK_MAXIMUM) == 0)
        data->order = CM_ORDER_MAXIMUM;
    else if (strcmp(argv[1], TK_EXIST) == 0)
        data->order = CM_ORDER_EXIST;
    else if (strcmp(argv[1], TK_SUM) == 0)
        data->order = CM_ORDER_SUM;
    else if (strcmp(argv[1], TK_INSERT) == 0)
        data->order = CM_ORDER_INSERT;
    else if (strcmp(argv[1], TK_INSERT_MANY) == 0)
        data->order = CM_ORDER_INSERT_MANY;
    else if (strcmp(argv[1], TK_PRINT) == 0)
        data->order = CM_ORDER_PRINT;
    else if (strcmp(argv[1], TK_LOCAL) == 0)
        data->order = CM_ORDER_LOCAL;
    else
        usage(argv[0], "commande inconnue");

    // deuxième vérification : nombre de paramètres correct ?
    if ((data->order == CM_ORDER_STOP) && (argc != 2))
        usage(argv[0], TK_STOP " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_HOW_MANY) && (argc != 2))
        usage(argv[0], TK_HOW_MANY " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MINIMUM) && (argc != 2))
        usage(argv[0], TK_MINIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MAXIMUM) && (argc != 2))
        usage(argv[0], TK_MAXIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_EXIST) && (argc != 3))
        usage(argv[0], TK_EXIST " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_SUM) && (argc != 2))
        usage(argv[0], TK_SUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_INSERT) && (argc != 3))
        usage(argv[0], TK_INSERT " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_INSERT_MANY) && (argc != 5))
        usage(argv[0], TK_INSERT_MANY " : il faut 3 arguments après la commande");
    if ((data->order == CM_ORDER_PRINT) && (argc != 2))
        usage(argv[0], TK_PRINT " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_LOCAL) && (argc != 7))
        usage(argv[0], TK_LOCAL " : il faut 5 arguments après la commande");

    // extraction des arguments
    if (data->order == CM_ORDER_EXIST)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT_MANY)
    {
        data->nb = strtol(argv[2], NULL, 10);
        data->min = strtof(argv[3], NULL);
        data->max = strtof(argv[4], NULL);
        if (data->nb < 1)
            usage(argv[0], TK_INSERT_MANY " : nb doit être strictement positif");
        if (data->max < data->min)
            usage(argv[0], TK_INSERT_MANY " : max ne doit pas être inférieur à min");
    }
    else if (data->order == CM_ORDER_LOCAL)
    {
        data->nbThreads = strtol(argv[2], NULL, 10);
        data->elt = strtof(argv[3], NULL);
        data->nb = strtol(argv[4], NULL, 10);
        data->min = strtof(argv[5], NULL);
        data->max = strtof(argv[6], NULL);
        if (data->nbThreads < 1)
            usage(argv[0], TK_LOCAL " : nbThreads doit être strictement positif");
        if (data->nb < 1)
            usage(argv[0], TK_LOCAL " : nb doit être strictement positif");
        if (data->max <= data->min)
            usage(argv[0], TK_LOCAL " : max ne doit être strictement supérieur à min");
    }
}


/************************************************************************
 * Partie multi-thread
 ************************************************************************/
//TODO Une structure pour les arguments à passer à un thread (aucune variable globale autorisée)
typedef struct
{
    int *result;
    float elt  ; 
    float * tab ; 
    int begin ; 
    int end ;
    pthread_mutex_t *mutex;
} ThreadData;
//TODO
// Code commun à tous les threads
// Un thread s'occupe d'une portion du tableau et compte en interne le nombre de fois
// où l'élément recherché est présent dans cette portion. On ajoute alors,
// en section critique, ce nombre au compteur partagé par tous les threads.
// Le compteur partagé est la variable "result" de "lauchThreads".
// A vous de voir les paramètres nécessaires  (aucune variable globale autorisée)
//END TODO
void * search (void * arg){
    ThreadData *data = (ThreadData *) arg;
    int ajout = 0;
    int ret;
    for(int  i = data->begin ; i< data->end ; i++){
        if( data->tab[i] == data->elt){
            ajout++ ; 
        }
    }

    ret = pthread_mutex_lock(data->mutex);
    assert(ret == 0);
    (*(data->result)) += ajout;
    ret = pthread_mutex_unlock(data->mutex);
    assert(ret == 0);
    return NULL ; 
}
void lauchThreads(const Data *data){ 

    //TODO déclarations nécessaires : mutex, ...
    int result = 0;
    float * tab = ut_generateTab(data->nb, data->min, data->max, 0) ;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    ThreadData  *datas = malloc(data->nbThreads*sizeof(ThreadData)) ;
    pthread_t *tabId = malloc(data->nbThreads*sizeof(pthread_t)) ;
    int sub_length = data->nb / data->nbThreads ; 
    for(int  i = 0 ; i<data->nbThreads ; i++){
        datas[i].result = &result ; 
        datas[i].elt = data->elt ; 
        datas[i].tab = tab ;
        datas[i].mutex = &mutex ;
        datas[i].begin = i*sub_length ; 
        int fin =datas[i].begin + sub_length; 
        while(fin> data->nb) {
            fin --;
        }
        datas[i].end = fin ; 
    }

    //TODO lancement des threads
    for( int  i = 0 ; i<data->nbThreads ;i++){
        int ret = pthread_create(&(tabId[i]), NULL, search, &(datas[i]));
        assert(ret == 0);
    }
    //TODO attente de la fin des threads
    for( int  i = 0 ; i<data->nbThreads ;i++){
        int ret = pthread_join(tabId[i], NULL);
        assert(ret == 0);
    }
    // résultat (result a été rempli par les threads)
    // affichage du tableau si pas trop gros
    if (data->nb <= 20)
    {
        printf("[");
        for (int i = 0; i < data->nb; i++)
        {
            if (i != 0)
                printf(" ");
            printf("%g", tab[i]);
        }
        printf("]\n");
    }
    // recherche linéaire pour vérifier
    int nbVerif = 0;
    for (int i = 0; i < data->nb; i++)
    {
        if (tab[i] == data->elt)
            nbVerif ++;
    }
    printf("Elément %g présent %d fois (%d attendu)\n", data->elt, result, nbVerif);
    if (result == nbVerif)
        printf("=> ok ! le résultat calculé par les threads est correct\n");
    else
        printf("=> PB ! le résultat calculé par les threads est incorrect\n");

    //TODO libération des ressources  
    free(tab);
    free(datas);  
    free(tabId);
    int ret = pthread_mutex_destroy(&mutex);
    assert(ret == 0);
}       


/************************************************************************
 * Partie communication avec le master
 ************************************************************************/
// envoi des données au master
void sendData(const Data *data)
{
    myassert(data != NULL, "pb !");   //TODO à enlever (présent pour éviter le warning)
    //TODO
    // - envoi de l'ordre au master (cf. CM_ORDER_* dans client_master.h)
    // - envoi des paramètres supplémentaires au master (pour CM_ORDER_EXIST,
    //   CM_ORDER_INSERT et CM_ORDER_INSERT_MANY)
    int ret ; 
    ret = write(data->fd_client_master ,&(data->order), sizeof(int) ) ; 
    assert(ret != -1 );
    if(data->order == CM_ORDER_EXIST || data->order ==CM_ORDER_INSERT )
    {
        ret = write(data->fd_client_master ,&(data->elt) ,sizeof(float));
        assert( ret != -1) ;
        
    }      
    if(data->order == CM_ORDER_INSERT_MANY ){
        ret = write(data->fd_client_master ,&(data->nb) , sizeof(int)  ); 
        assert (ret != -1) ; 
        ret = write(data->fd_client_master ,&(data->min) , sizeof(float) ); 
        assert(ret != -1);
        ret = write(data->fd_client_master ,&(data->max) , sizeof(float) ) ; 
        assert(ret != -1) ;
    }

    
    //END TODO

}

// attente de la réponse du master
void receiveAnswer(const Data *data)
{
    myassert(data != NULL, "pb !");   //TODO à enlever (présent pour éviter le warning)

    //TODO
    // - récupération de l'accusé de réception du master (cf. CM_ANSWER_* dans client_master.h)
    // - selon l'ordre et l'accusé de réception :
    //      . récupération de données supplémentaires du master si nécessaire
    // - affichage du résultat
    //END TODO
    int reponse ; 
    int ret ; 
    float result ;
    int nb_elt ; 
    int nb_elt_dist; 

    ret = read(data->fd_master_client , &reponse , sizeof(int) ) ; 
    assert(ret != -1 );
    switch(reponse){
        case CM_ANSWER_STOP_OK :
            printf("le stop  a etait effectué avec succés  \n");
            break;
        case CM_ANSWER_HOW_MANY_OK :
            ret = read(data->fd_master_client,&nb_elt,sizeof(int) ); 
            assert(ret != -1); 
            ret = read(data->fd_master_client,&nb_elt_dist,sizeof(int) );
            assert(ret != -1); 
            printf("le nombre de workers est %d  et le nombre d'elements distincts est %d ",nb_elt,nb_elt_dist);
            break;
        case CM_ANSWER_MINIMUM_OK :       
            ret=read(data->fd_master_client ,&result,sizeof(float));
            printf("le Minimum c'est  :( %g)  \n",result);
            break;
        case CM_ANSWER_MINIMUM_EMPTY :
            printf("l'ensemble est vide donc pas de min \n");
            break;
        case CM_ANSWER_MAXIMUM_OK:
            ret=read(data->fd_master_client ,&result,sizeof(float));
            printf("le Maximum c'est  : (%g)  \n",result);
            break;
        case CM_ANSWER_MAXIMUM_EMPTY :
            printf("l'ensemble est vide donc pas de max \n"); 
            break ; 
        case CM_ANSWER_EXIST_YES : 
            ret = read(data->fd_master_client ,&reponse , sizeof(int)); 
            assert(ret != -1) ; 
            printf(" Yes (%g) il exicte %d fois dans l'ensemble \n",data->elt , reponse);
            break; 
        case CM_ANSWER_EXIST_NO : 
            printf("Désolé l'element (%g)  n'est exicte pas  ", data->elt); 
            break ; 
        case CM_ANSWER_SUM_OK :
            ret = read(data->fd_master_client , &result , sizeof(float)); 
            assert( ret != -1) ; 
            printf("la somme de tous les éléments  est (%g) \n",result);
            break ; 
        case CM_ANSWER_INSERT_OK :
            printf("l'element (%g)  a bien etait inséeré ", data->elt); 
            break ; 
        case CM_ANSWER_INSERT_MANY_OK : 
            printf("le tableau de %d élement dont le min est (%g) et le max est (%g) vient d'étre  inseré avec succeés \n", data->nb , data->min , data->max);
            break;
        case CM_ANSWER_PRINT_OK : 
            printf("l'affichage  a bien etait efféfctue avec succées \n");
            break ;
        
    }

}


/************************************************************************
 * Fonction principale
 ************************************************************************/
int main(int argc, char * argv[])
{
    Data data;
    int ret ; 
    parseArgs(argc, argv, &data);

    if (data.order == CM_ORDER_LOCAL)
        lauchThreads(&data);
    else
    {
        //TODO
        // - entrer en section critique :
        //       . pour empêcher que 2 clients communiquent simultanément
        //       . le mutex est déjà créé par le master
        int ids = my_semget_access(PROJ_ID_CLIENT) ;
        entrerSC(ids);
        // - ouvrir les tubes nommés (ils sont déjà créés par le master)
        //       . les ouvertures sont bloquantes, il faut s'assurer que
        //         le master ouvre les tubes dans le même ordre
        data.fd_client_master = open(CLIENT_MASTER, O_WRONLY );
        assert(data.fd_client_master != -1 );
        data.fd_master_client = open(MASTER_CLIENT,O_RDONLY);
        assert(data.fd_master_client != -1 );
        //END TODO
        sendData(&data);
        receiveAnswer(&data);
        //TODO
        // - sortir de la section critique
        // - libérer les ressources (fermeture des tubes, ...)
        // - débloquer le master grâce à un second sémaphore (cf. ci-dessous)
        sortirSC(ids);
        ret=close(data.fd_master_client);
        assert(ret != -1 );
        ret =close(data.fd_client_master);
        assert(ret != -1);

        // Une fois que le master a envoyé la réponse au client, il se bloque
        // sur un sémaphore ; le dernier point permet donc au master de continuer
        ids = my_semget_access(PROJ_ID_MASTER) ;
        sortirSC(ids);
        // N'hésitez pa s à faire des fonctions annexes ; si la fonction main
        // ne dépassait pas une trentaine de lignes, ce serait bien.
    }
    
    return EXIT_SUCCESS;
}
