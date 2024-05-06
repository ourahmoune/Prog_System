#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "utils.h"
#include"config.h"
#include "myassert.h"
#include <assert.h>

#include "master_worker.h"


/************************************************************************
 * Données persistantes d'un worker
 ************************************************************************/
typedef struct
{
    // données internes (valeur de l'élément, cardinalité)
    float elt  ; 
    int card ; 
    bool bt_sub_left_empty   ; 
    bool bt_sub_right_empty   ;
    // communication avec le père (2 tubes) et avec le master (1 tube en écriture)
    int fd_worker_pere ; 
    int fd_pere_worker ; 
    int fd_worker_master ; 
    // communication avec le fils gauche s'il existe (2 tubes)
    int fd_pere_left[2] ; 
    int fd_left_pere[2] ; 
    // communication avec le fils droit s'il existe (2 tubes)
    int fd_pere_right[2]; 
    int fd_right_pere[2]; 
    //TODO
} Data;


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <elt> <fdIn> <fdOut> <fdToMaster>\n", exeName);
    fprintf(stderr, "   <elt> : élément géré par le worker\n");
    fprintf(stderr, "   <fdIn> : canal d'entrée (en provenance du père)\n");
    fprintf(stderr, "   <fdOut> : canal de sortie (vers le père)\n");
    fprintf(stderr, "   <fdToMaster> : canal de sortie directement vers le master\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

static void parseArgs(int argc, char * argv[], Data *data)
{
    myassert(data != NULL, "il faut l'environnement d'exécution");

    if (argc != 5)
        usage(argv[0], "Nombre d'arguments incorrect");

    //TODO initialisation data
    float elt = strtof(argv[1], NULL);
    int fdIn = strtol(argv[2], NULL, 10);
    int fdOut = strtol(argv[3], NULL, 10);
    int fdToMaster = strtol(argv[4], NULL, 10);
    data->card = 1;
    data->bt_sub_left_empty= true; 
    data->bt_sub_right_empty = true ;
    data->elt = elt ; 
    data->fd_worker_pere = fdIn;
    data->fd_pere_worker = fdOut;
    data->fd_worker_master = fdToMaster ; 
    //TODO (à enlever) comment récupérer les arguments de la ligne de commande
    //END TODO
}

/************************************************************************
 * Stop 
 ************************************************************************/
void stopAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre stop\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - envoyer au worker gauche ordre de fin (cf. master_worker.h)
    // - envoyer au worker droit ordre de fin (cf. master_worker.h)
    // - attendre la fin des deux fils
    //END TODO
    int ret ;
    int order =MW_ORDER_STOP;
    if(data->bt_sub_left_empty && data->bt_sub_right_empty){

    }else{ // soit fg non soit fd non vide soit les deux 
      if(data->bt_sub_left_empty){ //fd nest pas vide 
        ret  = write(data->fd_pere_right[1],&order,sizeof(int));
        assert(ret != -1); 
        wait(NULL);
      }else{//fg nest pas vide 
        if(data->bt_sub_right_empty){
          ret = write(data->fd_pere_left[1],&order,sizeof(int));
          assert(ret != -1);
          wait(NULL);
        }else{//les deux non vide
          ret = write(data->fd_pere_left[1],&order,sizeof(int));
          assert(ret != -1); 
          ret  = write(data->fd_pere_right[1],&order,sizeof(int));
          assert(ret != -1); 
          wait(NULL);
          wait(NULL);
        }
      }
    }
}


/************************************************************************
 * Combien d'éléments
 ************************************************************************/
static void howManyAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre how many\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    //       . envoyer ordre howmany (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir deux résultats (nb elts, nb elts distincts) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer les résultats (les cumuls des deux quantités + la valeur locale) au père
    //END TODO
    int ret ; 
    int nb_elt_worker; 
    int nb_elt_dist_worker ;
    int somme_nb_elt=data->card;
    int somme_nb_elt_dist=1;
    int ack=MW_ANSWER_HOW_MANY;
    int order=MW_ORDER_HOW_MANY;

    if(!(data->bt_sub_left_empty)){
      ret=write(data->fd_pere_left[1],&order,sizeof(int));
      assert( ret != -1); 
      ret= read(data->fd_left_pere[0],&ack,sizeof(int)); 
      assert(ret != -1);
      if(ack==MW_ANSWER_HOW_MANY ){
        ret= read(data->fd_left_pere[0],&nb_elt_worker,sizeof(int)); 
        assert(ret != -1); 
        ret= read(data->fd_left_pere[0],&nb_elt_dist_worker,sizeof(int)); 
        assert(ret != -1); 
        somme_nb_elt = somme_nb_elt +nb_elt_worker; 
        somme_nb_elt_dist = somme_nb_elt_dist + nb_elt_dist_worker;
 
      }
    }
    if(!(data->bt_sub_right_empty)){
      ret=write(data->fd_pere_right[1],&order,sizeof(int));
      assert(ret!= -1); 
      ret= read(data->fd_right_pere[0],&ack,sizeof(int)); 
      assert(ret != -1); 
        if(ack== MW_ANSWER_HOW_MANY){
          ret=read(data->fd_right_pere[0],&nb_elt_worker,sizeof(int));
          assert(ret != -1);
          ret=read(data->fd_right_pere[0],&nb_elt_dist_worker,sizeof(int));
          assert(ret != -1);
          somme_nb_elt=somme_nb_elt + nb_elt_worker;
          somme_nb_elt_dist= somme_nb_elt_dist + nb_elt_dist_worker;

        }
    }

    ret = write(data->fd_worker_pere,&ack, sizeof(int)); 
    assert( ret != -1); 
    ret = write(data->fd_worker_pere,&somme_nb_elt,sizeof(int)); 
    assert(ret != -1);
    ret = write(data->fd_worker_pere,&somme_nb_elt_dist,sizeof(int)); 
    assert(ret != -1); 

    
    
}


/************************************************************************
 * Minimum
 ************************************************************************/
static void minimumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre minimum\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche n'existe pas (on est sur le minimum)
    //       . envoyer l'accusé de réception au master (cf. master_worker.h)
    //       . envoyer l'élément du worker courant au master
    // - sinon
    //       . envoyer au worker gauche ordre minimum (cf. master_worker.h)
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO
    int ret , ack ,order ; 
    if(data->bt_sub_left_empty){
      ack=MW_ANSWER_MINIMUM;
      ret= write(data->fd_worker_master,&ack,sizeof(int));
      assert(ret != -1);
      ret = write(data->fd_worker_master,&(data->elt),sizeof(float)); 
      assert(ret != -1); 
    }else{
      order = MW_ORDER_MINIMUM ; 
      ret = write (data->fd_pere_left[1],&order,sizeof(int));
      assert( ret != -1); 

    }
}


/************************************************************************
 * Maximum
 ************************************************************************/
static void maximumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre maximum\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // cf. explications pour le minimum
    //END TODO
    int ret , ack ,order ; 
    if(data->bt_sub_right_empty){
      ack=MW_ANSWER_MAXIMUM;
      ret= write(data->fd_worker_master,&ack,sizeof(int));
      assert(ret != -1);
      ret = write(data->fd_worker_master,&(data->elt),sizeof(float)); 
      assert(ret != -1); 
    }else{
      order = MW_ORDER_MAXIMUM ; 
      ret = write (data->fd_pere_right[1],&order,sizeof(int));
      assert( ret != -1); 

    }
}

/************************************************************************
 * Existence
 ************************************************************************/
static void existAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre exist\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du père
    // - si élément courant == élément à tester
    //       . envoyer au master l'accusé de réception de réussite (cf. master_worker.h)
    //       . envoyer cardinalité de l'élément courant au master
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant)
    //       . envoyer au worker gauche ordre exist (cf. master_worker.h)
    //       . envoyer au worker gauche élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    // - sinon (donc elt à tester > elt courant)
    //       . envoyer au worker droit ordre exist (cf. master_worker.h)
    //       . envoyer au worker droit élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO
    int ret ,order , ack ; 
    float elt ;  
    ret = read (data->fd_pere_worker,&elt,sizeof(float));
    assert(ret != -1) ; 
    if(elt == data->elt){
      ack=MW_ANSWER_EXIST_YES;
      ret= write(data->fd_worker_master,&ack , sizeof(int));
      assert(ret != -1);
      ret= write(data->fd_worker_master,&(data->card) ,sizeof(int));
      assert(ret != -1);
    }else{
      if(elt< data->elt){
        if(data->bt_sub_left_empty){
          ack=MW_ANSWER_EXIST_NO;
          ret= write(data->fd_worker_master,&ack , sizeof(int));
          assert(ret != -1);
        }else{
          order = MW_ORDER_EXIST;
          ret =write(data->fd_pere_left[1],&order,sizeof(int));
          assert(ret != -1); 
          ret = write (data->fd_pere_left[1],&elt,sizeof(float));
          assert(ret != -1) ; 
        }
      }else{//si elt > data->elt
        if(data->bt_sub_right_empty){
          ack=MW_ANSWER_EXIST_NO;
          ret= write(data->fd_worker_master,&ack , sizeof(int));
          assert(ret != -1);
        }else{
          order = MW_ORDER_EXIST;
          ret =write(data->fd_pere_right[1],&order,sizeof(int));
          assert(ret != -1); 
          ret = write (data->fd_pere_right[1],&elt,sizeof(float));
          assert(ret != -1) ; 
        }
      }
    }



}


/************************************************************************
 * Somme
 ************************************************************************/
static void sumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre sum\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    //       . envoyer ordre sum (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir résultat (somme du fils) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer le résultat (le cumul des deux quantités + la valeur locale) au père
    //END TODO
    int ack , order , ret ; 
    float somme_worker_gauche ; 
    float somme_worker_droite; 
    float somme_total =data->elt * data->card ; 
    order =MW_ORDER_SUM ;
    if(!(data->bt_sub_left_empty)){
      ret = write(data->fd_pere_left[1],&order, sizeof(int)); 
      assert(ret != -1 );
      ret = read(data->fd_left_pere[0], &ack, sizeof(int) ); 
      assert( ret != -1) ; 
      if(ack ==MW_ANSWER_SUM){
        ret = read(data->fd_left_pere[0], &somme_worker_gauche , sizeof(float)); 
        assert(ret != -1) ; 
        somme_total = somme_total + somme_worker_gauche ;
      }
    }
    if(!(data->bt_sub_right_empty)){
      ret = write(data->fd_pere_right[1],&order, sizeof(int)); 
      assert(ret != -1 );
      ret = read(data->fd_right_pere[0], &ack, sizeof(int) ); 
      assert( ret != -1) ; 
      if(ack ==MW_ANSWER_SUM){
        ret = read(data->fd_right_pere[0], &somme_worker_droite , sizeof(float)); 
        assert(ret != -1) ; 
        somme_total = somme_total + somme_worker_droite ;
      }
    }
    ack=MW_ANSWER_SUM;
    ret = write(data->fd_worker_pere,&ack,sizeof(int)); 
    assert ( ret  != -1 );
    ret = write(data->fd_worker_pere ,&somme_total , sizeof(float)); 
}


/************************************************************************
 * Insertion d'un nouvel élément
 ************************************************************************/
static void insertAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre insert\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à insérer en provenance du père
    // - si élément courant == élément à tester
    //       . incrémenter la cardinalité courante
    //       . envoyer au master l'accusé de réception (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . créer un worker à gauche avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . créer un worker à droite avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à insérer < elt courant)
    //       . envoyer au worker gauche ordre insert (cf. master_worker.h)
    //       . envoyer au worker gauche élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    // - sinon (donc elt à insérer > elt courant)
    //       . envoyer au worker droit ordre insert (cf. master_worker.h)
    //       . envoyer au worker droit élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    //END TODO

    float  elt ;
    int ret ; 
    int ack ;
    pid_t retFork;
   
    

    ret = read(data->fd_pere_worker,&elt,sizeof(float));
    assert(ret != -1 );
    if(elt == data->elt){
      data->card ++ ; 
      ack=MW_ANSWER_INSERT;
      ret = write(data->fd_worker_master,&ack,sizeof(int));
      assert(ret != -1);
    }else if(elt<data->elt){
      if(data->bt_sub_left_empty){
        data->bt_sub_left_empty = false;
        ret=pipe(data->fd_pere_left);
        assert(ret != -1);
        ret=pipe(data->fd_left_pere);
        assert(ret != -1);
        int fdIn  = data->fd_left_pere[1]; 
        int fdOut =data->fd_pere_left[0];  
        int fdToMaster = data->fd_worker_master;
        retFork= fork();
        if(retFork == 0 ){
          char *argv[6] ; 
          char * char_elt =strFormat_of_float(elt);//strFormat_of_float est dans utils.c
          char * char_fdIn=strFormat_of_int(fdIn);//strFormat_of_int est dans utils.c
          char * char_fdOut=strFormat_of_int(fdOut);
          char * char_fdToMaster=strFormat_of_int(fdToMaster);
          argv[0]=WORKER;//WORKER est defnie dans master_worker.h
          argv[1]=char_elt ; 
          argv[2]=char_fdIn; 
          argv[3]=char_fdOut; 
          argv[4]= char_fdToMaster;
          argv[5]=NULL;
          execv(argv[0],argv);
          assert(false);
        }
      }else{
        
        int ordre =MW_ORDER_INSERT ;
        ret= write(data->fd_pere_left[1],&ordre,sizeof(int)); 
        assert(ret != -1); 
        ret = write(data->fd_pere_left[1] , &elt , sizeof(float)) ; 
        assert(ret != -1);
        
      }

    }else {//valeur sup a la racine 
      if(data->bt_sub_right_empty){
        data->bt_sub_right_empty = false;
        ret=pipe(data->fd_pere_right);
        assert(ret != -1);
        ret=pipe(data->fd_right_pere);
        assert(ret != -1);
        retFork= fork();
        if(retFork == 0 ){
          char *argv[6] ;
          int fdIn  = data->fd_right_pere[1]; 
          int fdOut =data->fd_pere_right[0];  
          int fdToMaster = data->fd_worker_master;
          char * char_elt =strFormat_of_float(elt);
          char * char_fdIn=strFormat_of_int(fdIn);
          char *  char_fdOut=strFormat_of_int(fdOut);
          char * char_fdToMaster=strFormat_of_int(fdToMaster);
          argv[0]=WORKER;
          argv[1]=char_elt ; 
          argv[2]=char_fdIn; 
          argv[3]=char_fdOut; 
          argv[4]= char_fdToMaster;
          argv[5]=NULL;
          execv(argv[0],argv);
          assert(false);
        }
      }else{
        int ordre =MW_ORDER_INSERT ;
        ret= write(data->fd_pere_right[1],&ordre,sizeof(int)); 
        assert(ret != -1); 
        ret = write(data->fd_pere_right[1], &elt , sizeof(float)) ; 
        assert(ret != -1);
        
      }

    }
}


/************************************************************************
 * Affichage
 ************************************************************************/
static void printAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre print\n", getpid(), getppid(), data->elt /*TODO élément*/);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - afficher l'élément courant avec sa cardinalité
    // - si le fils droit existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    //END TODO
    int ret , order , ack ;
    if(data->bt_sub_left_empty  && data->bt_sub_right_empty){
      ack = MW_ANSWER_PRINT;
      printf("  (%f %d )\n", data->elt,data->card );
      ret= write(data->fd_worker_pere,&ack,sizeof(int));
      assert(ret != -1);
      
    }else{// soit fg n'est pas vide soit fd n'est pas vide soit les deux sont pas vide 
      if(data->bt_sub_right_empty) {// fg n'est pas vide 
        order=MW_ORDER_PRINT ;
        ack =MW_ANSWER_PRINT ;
        ret = write(data->fd_pere_left[1],&order,sizeof(int));
        assert(ret != -1) ; 
        ret = read(data->fd_left_pere[0],&ack,sizeof(int)); 
        if(ack ==MW_ANSWER_PRINT ){
          printf(" ( %f %d )\n",data->elt , data->card);
          ret= write(data->fd_worker_pere,&ack,sizeof(int));
          assert(ret != -1);
        }
      }else{//fd n'est pas vide 
        if(data->bt_sub_left_empty){ //fd n'est pas vide et fg vide
          order=MW_ORDER_PRINT ;
          ack =MW_ANSWER_PRINT ;
          printf(" ( %f %d )\n",data->elt , data->card);
          ret = write(data->fd_pere_right[1],&order,sizeof(int));
          assert(ret != -1) ; 
          ret = read(data->fd_right_pere[0],&ack,sizeof(int));
          assert(ret != -1); 
          ret= write(data->fd_worker_pere,&ack,sizeof(int));
          assert(ret != -1);
        }else{
          //fd n'est pas vide fg aussi 
          order=MW_ORDER_PRINT ;
          ack =MW_ANSWER_PRINT ;
          ret = write(data->fd_pere_left[1],&order,sizeof(int));
          assert(ret != -1) ; 
          ret = read(data->fd_left_pere[0],&ack,sizeof(int)); 
          if(ack ==MW_ANSWER_PRINT ){
            printf(" ( %f %d )\n",data->elt , data->card);
            ret = write(data->fd_pere_right[1],&order,sizeof(int));
            assert(ret != -1) ; 
            ret = read(data->fd_right_pere[0],&ack,sizeof(int));
            assert(ret != -1); 
            ret= write(data->fd_worker_pere,&ack,sizeof(int));
            assert(ret != -1);
          }

        }

      }
      
    }

}


/************************************************************************
 * Boucle principale de traitement
 ************************************************************************/
void loop(Data *data)
{
    bool end = false;
    int ret ;
    while (! end)
    {
        int order =MW_ORDER_STOP;   //TODO pour que ça ne boucle pas, mais recevoir l'ordre du père
        ret = read(data->fd_pere_worker ,&order,sizeof(int)) ; 
        assert( ret != -1);
        switch(order)
        {
          case MW_ORDER_STOP:
            stopAction(data);
            end = true;
            break;
          case MW_ORDER_HOW_MANY:
            howManyAction(data);
            break;
          case MW_ORDER_MINIMUM:
            minimumAction(data);
            break;
          case MW_ORDER_MAXIMUM:
            maximumAction(data);
            break;
          case MW_ORDER_EXIST:
            existAction(data);
            break;
          case MW_ORDER_SUM:
            sumAction(data);
            break;
          case MW_ORDER_INSERT:
            insertAction(data);
            break;
          case MW_ORDER_PRINT:
            printAction(data);
            break;
          default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }

        TRACE3("    [worker (%d, %d) {%g}] : fin ordre\n", getpid(), getppid(), data->elt /*TODO élément*/);
    }
}


/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char * argv[])
{
    Data data;
    parseArgs(argc, argv, &data);
    TRACE3("    [worker (%d, %d) {%g}] : début worker\n", getpid(), getppid(), data.elt /*TODO élément*/);
    int ret ; 
    //TODO envoyer au master l'accusé de réception d'insertion (cf. master_worker.h)
    int rep = MW_ANSWER_INSERT ; 
    ret=write(data.fd_worker_master,&rep ,sizeof(int));
    assert ( ret != -1) ; 
    //TODO note : en effet si je suis créé c'est qu'on vient d'insérer un élément : moi
    loop(&data);
    //TODO fermer les tubes
    TRACE3("    [worker (%d, %d) {%g}] : fin worker\n", getpid(), getppid(), data.elt /*TODO élément*/);
    return EXIT_SUCCESS;
}
