#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utils.h"
#include "myassert.h"
#include "config.h"

#include "client_master.h"
#include "master_worker.h"

#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/
typedef struct
{
    //TODO
    // communication avec le client
    int fd_master_client ; 
    int fd_client_master ;
    // données internes
    bool is_empty ;
    // communication avec le premier worker (double tubes)
    int  fd_master_worker1[2] ; 
    int  fd_worker1_master[2]  ; 
    // communication en provenance de tous les workers (un seul tube en lecture)
    int fd_workers_master[2] ; //en lecture   

} Data;

/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

/************************************************************************
 * initialisation complète
 ************************************************************************/
void init(Data *data)
{
    myassert(data != NULL, "il faut l'environnement d'exécution");
    //TODO initialisation data
    int ret ;
    data->is_empty = true;
    ret = pipe(data->fd_master_worker1) ; 
    assert( ret != -1 ); 
    ret = pipe (data->fd_worker1_master) ; 
    assert( ret != -1) ;
    ret = pipe(data->fd_workers_master); 
    assert(ret != - 1); 
    
}

/************************************************************************
 * fin du master
 ************************************************************************/
void orderStop(Data *data)
{
    TRACE0("[master] ordre stop\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int ret ; 
    
    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre de fin (cf. master_worker.h)
    // - attendre sa fin
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
    if(data->is_empty){
      int rep = CM_ANSWER_STOP_OK ; 
      ret = write(data->fd_master_client,&rep,sizeof(int)); 
      assert(ret != -1); 
    }else{
      int ordre =MW_ORDER_STOP ;
      ret = write (data->fd_master_worker1[1],&ordre,sizeof(int)); 
      wait(NULL);
      assert(ret != -1);
      int rep = CM_ANSWER_STOP_OK ; 
      ret = write(data->fd_master_client,&rep,sizeof(int)); 
      assert(ret != -1); 
    }
}

/************************************************************************
 * quel est la cardinalité de l'ensemble
 ************************************************************************/
void orderHowMany(Data *data)
{
    TRACE0("[master] ordre how many\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre howmany (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultats (deux quantités) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    // - envoyer les résultats au client
    //END TODO
    int ack = CM_ANSWER_HOW_MANY_OK ;
    int order = MW_ORDER_HOW_MANY ;  
    int ret ;
    int nb_elt , nb_elt_dist;
    if(data->is_empty){
      ret = write(data->fd_master_client ,&ack, sizeof(int)); 
      assert(ret != -1); 
      nb_elt=0; 
      nb_elt_dist=0;
      ret = write(data->fd_master_client ,&nb_elt, sizeof(int)); 
      assert(ret != -1);
      ret = write(data->fd_master_client ,&nb_elt_dist, sizeof(int)); 
      assert(ret != -1);  
    }else{
      ret = write(data->fd_master_worker1[1],&order,sizeof(int)); 
      assert(ret != -1);
      ret = read (data->fd_worker1_master[0],&ack , sizeof(int));
      assert(ret != -1) ; 
      if(ack == MW_ANSWER_HOW_MANY)  {
        ret= read(data->fd_worker1_master[0],&nb_elt,sizeof(int)); 
        assert(ret != -1); 
        ret= read(data->fd_worker1_master[0],&nb_elt_dist,sizeof(int)); 
        assert(ret != -1); 
        ret = write(data->fd_master_client ,&ack, sizeof(int)); 
        assert(ret != -1); 
        ret = write(data->fd_master_client ,&nb_elt, sizeof(int)); 
        assert(ret != -1);
        ret = write(data->fd_master_client ,&nb_elt_dist, sizeof(int)); 
        assert(ret != -1); 
      }
    }
}

/************************************************************************
 * quel est la minimum de l'ensemble
 ************************************************************************/
void orderMinimum(Data *data)
{
    TRACE0("[master] ordre minimum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int ret ,ack ;

    //TODO
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    // - sinon
    //       . envoyer au premier worker ordre minimum (cf. master_worker.h)
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . recevoir résultat (la valeur) venant du worker concerné
    //       . envoyer l'accusé de réception au client (cf. client_master.h)
    //       . envoyer le résultat au client
    //END TODO
    if(data->is_empty){
      ack = CM_ANSWER_MINIMUM_EMPTY ;
      ret= write(data->fd_master_client,&ack , sizeof(int)) ;
      assert(ret != -1);
    }else{
      int ordre = MW_ORDER_MINIMUM ;
      ret=write(data->fd_master_worker1[1],&ordre,sizeof(int));
      assert(ret != -1);
      ret=read(data->fd_workers_master[0],&ack,sizeof(int));
      if(ack == MW_ANSWER_MINIMUM){
        int result ; 
        ret= read(data->fd_workers_master[0],&result,sizeof(float)); 
        assert(ret != -1); 
        ack = CM_ANSWER_MINIMUM_OK ;
        ret = write(data->fd_master_client,&ack,sizeof(int));
        assert(ret != -1) ; 
        ret= write(data->fd_master_client,&result,sizeof(float)); 
        assert(ret != -1);
      }
    }
}

/************************************************************************
 * quel est la maximum de l'ensemble
 ************************************************************************/
void orderMaximum(Data *data)
{
    TRACE0("[master] ordre maximum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // cf. explications pour le minimum
    //END TODO
    int ret ,ack ;
    if(data->is_empty){
      ack = CM_ANSWER_MAXIMUM_EMPTY ;
      ret= write(data->fd_master_client,&ack , sizeof(int)) ;
      assert(ret != -1);
    }else{
      int ordre = MW_ORDER_MAXIMUM ;
      ret=write(data->fd_master_worker1[1],&ordre,sizeof(int));
      assert(ret != -1);
      ret=read(data->fd_workers_master[0],&ack,sizeof(int));
      if(ack == MW_ANSWER_MAXIMUM){
        int result ; 
        ret= read(data->fd_workers_master[0],&result,sizeof(float)); 
        assert(ret != -1); 
        ack = CM_ANSWER_MAXIMUM_OK ;
        ret = write(data->fd_master_client,&ack,sizeof(int));
        assert(ret != -1) ; 
        ret= write(data->fd_master_client,&result,sizeof(float)); 
        assert(ret != -1);
      }
    }
}


/************************************************************************
 * test d'existence
 ************************************************************************/
void orderExist(Data *data)
{
    TRACE0("[master] ordre existence\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du client
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    // - sinon
    //       . envoyer au premier worker ordre existence (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à tester
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . si élément non présent
    //             . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    //       . sinon
    //             . recevoir résultat (une quantité) venant du worker concerné
    //             . envoyer l'accusé de réception au client (cf. client_master.h)
    //             . envoyer le résultat au client
    //END TODO
    int ret , order , ack ; 
    float elt ; 
    ret = read(data->fd_client_master,&elt,sizeof(float)); 
    assert(ret !=-1);
    if(data->is_empty){
      ack= CM_ANSWER_EXIST_NO;
      ret = write(data->fd_master_client,&ack,sizeof(int));
      assert(ret != -1); 
    }else{
      order= MW_ORDER_EXIST ;
      ret = write(data->fd_master_worker1[1],&order,sizeof(int)); 
      assert(ret != -1 );
      ret = write(data->fd_master_worker1[1],&elt,sizeof(float)); 
      assert(ret != -1 );
      ret= read (data->fd_workers_master[0],&ack,sizeof(int));
      assert(ret != -1) ;
      if(ack==MW_ANSWER_EXIST_NO){
        ack=CM_ANSWER_EXIST_NO ;
        ret= write(data->fd_master_client,&ack,sizeof(int));
        assert(ret != -1);
      }else{
        ack = CM_ANSWER_EXIST_YES ; 
        int card ;
        ret = read (data->fd_workers_master[0],&card , sizeof(int));
        assert(ret != -1); 
        ret = write (data->fd_master_client,&ack , sizeof(int));
        assert(ret != -1) ; 
        ret = write (data->fd_master_client ,&card , sizeof(int));
        assert(ret != -1);

      }

    }

}

/************************************************************************
 * somme
 ************************************************************************/
void orderSum(Data *data)
{
    TRACE0("[master] ordre somme\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker) : la somme est alors 0
    // - envoyer au premier worker ordre sum (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultat (la somme) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    // - envoyer le résultat au client    
    //END TODO
    int ack , order , ret  ;
    float somme =0 ; 
    float somme_worker ;
    if(!(data->is_empty)){
      order =MW_ORDER_SUM ; 
      ret = write(data->fd_master_worker1[1] , &order , sizeof(int)); 
      assert(ret != -1) ;  
      ret = read (data->fd_worker1_master[0] , &ack , sizeof(int)); 
      assert( ret != -1) ;
      if(ack == MW_ANSWER_SUM) {
        ret = read(data->fd_worker1_master[0] ,&somme_worker,sizeof(float));
        assert(ret != -1); 
        somme = somme + somme_worker ; 
      }
    }
    ack =CM_ANSWER_SUM_OK ;
    ret = write(data->fd_master_client ,&ack , sizeof(int)); 
    assert(ret != -1)  ; 
    ret = write(data->fd_master_client ,&somme , sizeof(float)); 
    assert(ret != -1)  ; 
}

/************************************************************************
 * insertion d'un élément
 ************************************************************************/

//TODO voir si une fonction annexe commune à orderInsert et orderInsertMany est justifiée

void orderInsert(Data *data)
{ 
    TRACE0("[master] ordre insertion\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");
    int ret ;
    float elt;
    int ack ; 
    //TODO
    // - recevoir l'élément à insérer en provenance du client
    // - si ensemble vide (pas de premier worker)
    //       . créer le premier worker avec l'élément reçu du client
    // - sinon
    //       . envoyer au premier worker ordre insertion (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à insérer
    // - recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO 

    ret = read(data->fd_client_master,&elt, sizeof(float)) ; 
    assert(ret != -1); 
    pid_t retFork ;
    if(data->is_empty){  //ensemble vide

      retFork = fork() ; 
        if( retFork == 0) {
          char * argv[6];
          char *char_elt  =strFormat_of_float(elt); // strFormat_of_float est dans utils.c
          int fdIn  = data->fd_worker1_master[1]; // le tube dans le quelle le worker il ecrit
          int fdOut =data->fd_master_worker1[0];  // le tube dans le quelle le worker il lit 
          int fdToMaster = data->fd_workers_master[1];
          char *char_fdIn =strFormat_of_int(fdIn); // strFormat_of_int est dans utils.c
          char *char_fdOut =strFormat_of_int(fdOut); 
          char * char_fdToMaster=strFormat_of_int(fdToMaster);
          argv[0]= WORKER ; //WORKER est defnie dans master_worker.h
          argv[1]=  char_elt;
          argv[2]= char_fdIn  ;
          argv[3]= char_fdOut ;
          argv[4]= char_fdToMaster ;
          argv[5]= NULL;
          execv(argv[0] , argv);
        }else{
          // - recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
          ret = read(data->fd_workers_master[0],&ack , sizeof(int));
          assert(ret != -1 );
          if(ack == MW_ANSWER_INSERT) {
            ack =CM_ANSWER_INSERT_OK ;
            //envoyer l'accusé de réception au client (cf. client_master.h)
            ret = write(data->fd_master_client , &ack,sizeof(int));
            assert(ret != -1);
            data->is_empty = false ;

          } 
      
        }
    }
    else{//ensemble pas vide
      int order = MW_ORDER_INSERT ;
      ret = write(data->fd_master_worker1[1],&order,sizeof(int));
      assert(ret != -1 ); 
      ret = write(data->fd_master_worker1[1],&elt,sizeof(float)) ;
      assert(ret  != -1);
        // idee fork au debut de test si l'ensemble est vide 
          ret = read(data->fd_workers_master[0],&ack , sizeof(int));
          assert(ret != -1 );
          if(ack == MW_ANSWER_INSERT) {
            ack =CM_ANSWER_INSERT_OK ;
            //envoyer l'accusé de réception au client (cf. client_master.h)
            ret = write(data->fd_master_client , &ack,sizeof(int));
            assert(ret != -1);
            data->is_empty = false ;
          } //

    }


}
/************************************************************************
 * insertion d'un tableau d'éléments
 ************************************************************************/
void orderInsertMany(Data *data)
{
    TRACE0("[master] ordre insertion tableau\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir le tableau d'éléments à insérer en provenance du client
    // - pour chaque élément du tableau
    //       . l'insérer selon l'algo vu dans orderInsert (penser à factoriser le code)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
    int ret, ack , nb;
    float min , max ,elt ; 
    ret = read(data->fd_client_master,&nb,sizeof(int)); 
    assert(ret != -1) ; 
    ret = read(data->fd_client_master,&min,sizeof(float)); 
    assert(ret != -1) ; 
    ret = read(data->fd_client_master,&max,sizeof(float)); 
    assert(ret != -1) ; 
    float * tab = ut_generateTab(nb, min ,max,PRECISION);
    //PRECISION est definie a 3 dans util.h
    for(int i=0 ; i<nb ; i++){
      elt =  tab[i];
      pid_t retFork ;
      if(data->is_empty){  //ensemble vide

        retFork = fork() ; 
          if( retFork == 0) {
            char * argv[6];
            char * char_elt =strFormat_of_float(elt); // strFormat_of_float est dans utils.c
            int fdIn  = data->fd_worker1_master[1]; // le tube dans le quelle le worker il ecrit
            int fdOut =data->fd_master_worker1[0];  // le tube dans le quelle le worker il lit 
            int fdToMaster = data->fd_workers_master[1];
            char *char_fdIn =strFormat_of_int(fdIn); // strFormat_of_int est dans utils.c
            char *char_fdOut =strFormat_of_int(fdOut); 
            char * char_fdToMaster=strFormat_of_int(fdToMaster);
            argv[0]= WORKER ; //WORKER est defnie dans master_worker.h
            argv[1]=  char_elt;
            argv[2]= char_fdIn  ;
            argv[3]= char_fdOut ;
            argv[4]= char_fdToMaster ;
            argv[5]= NULL;
            execv(argv[0] , argv);
          }else{
            // - recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
            ret = read(data->fd_workers_master[0],&ack , sizeof(int));
            assert(ret != -1 ); 
            data->is_empty = false ;     
          }
        }
      else{//ensemble pas vide
        int order = MW_ORDER_INSERT ;
        ret = write(data->fd_master_worker1[1],&order,sizeof(int));
        assert(ret != -1 ); 
        ret = write(data->fd_master_worker1[1],&elt,sizeof(float)) ;
        assert(ret  != -1);
          // idee fork au debut de test si l'ensemble est vide 
        ret = read(data->fd_workers_master[0],&ack , sizeof(int));
        assert(ret != -1 );
        if(ack == MW_ANSWER_INSERT) {
          ack =CM_ANSWER_INSERT_OK ;
          //envoyer l'accusé de réception au client (cf. client_master.h)S
          data->is_empty = false ;
        } //

      }
    }
    ack = CM_ANSWER_INSERT_MANY_OK ;
    ret = write(data->fd_master_client ,&ack , sizeof(int) );
    assert(ret != -1) ; 
    free(tab);

    

}


/************************************************************************
 * affichage ordonné
 ************************************************************************/
void orderPrint(Data *data)
{
    TRACE0("[master] ordre affichage\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre print (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    //   note : ce sont les workers qui font les affichages
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
    int order , ack;
    int ret ;
    if(!(data->is_empty)){ 
      order = MW_ORDER_PRINT;
      ret= write(data->fd_master_worker1[1],&order, sizeof(int));
      assert(ret != -1);
      ret = read(data->fd_worker1_master[0],&ack, sizeof(int));
      assert(ret != -1); 
    }
    order= CM_ANSWER_PRINT_OK;
    ret = write(data->fd_master_client , &order , sizeof(int)); 
    assert(ret != -1);

}

/************************************************************************
 * boucle principale de communicatiret= read (data->fd_client_master , &(data->nb) , sizeof(int ));on avec le client
 ************************************************************************/
void loop(Data *data)
{
    bool end = false;
    int ret ;
    init(data);

    while (end == false)
    {
        //TODO ouverture des tubes avec le client (cf. explications dans client.c)
        data->fd_client_master=open(CLIENT_MASTER ,O_RDONLY);
        data->fd_master_client=open(MASTER_CLIENT,O_WRONLY);
        int order = CM_ORDER_STOP; //TODO pour que ça ne boucle pas, mais recevoir l'ordre du client
        ret = read(data->fd_client_master,&order , sizeof(int)); 
        assert(ret != -1 );
         
        switch(order)
        {
          case CM_ORDER_STOP:
            orderStop(data);
            end = true;
            break;
          case CM_ORDER_HOW_MANY:
            orderHowMany(data);
            break;
          case CM_ORDER_MINIMUM:
            orderMinimum(data);
            break;
          case CM_ORDER_MAXIMUM:
            orderMaximum(data);
            break;
          case CM_ORDER_EXIST:
            orderExist(data);
            break;
          case CM_ORDER_SUM:
            orderSum(data);
            break;
          case CM_ORDER_INSERT:
            orderInsert(data);
            break;
          case CM_ORDER_INSERT_MANY:
            orderInsertMany(data);
            break;
          case CM_ORDER_PRINT:
            orderPrint(data);
            break;
          default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }
        //TODO fermer les tubes nommés
        
        
        ret = close(data->fd_master_client) ;
        assert(ret != -1 ) ;
        ret = close(data->fd_client_master);
        assert(ret != -1 ) ; 
        //     il est important d'ouvrir et fermer les tubes nommés à chaque itération
        //     voyez-vous pourquoi ?
        //TODO attendre ordre du client avant de continuer (sémaphore pour une précédence)
        TRACE0("[master] fin ordre\n");
        int ids = my_semget_access(PROJ_ID_MASTER) ; 
        entrerSC(ids);
    }
}

/************************************************************************
 * Fonction principale
 ************************************************************************/

//TODO N'hésitez pas à faire des fonctions annexes ; si les fonctions main
//TODO et loop pouvaient être "courtes", ce serait bien

int main(int argc, char * argv[])
{
    if (argc != 1)
        usage(argv[0], NULL);

    TRACE0("[master] début\n");

    Data data;
    int ret ; 
    //TODO
    // - création des sémaphores
    // - création des tubes nommés
    mkfifo(MASTER_CLIENT,0644);
    mkfifo(CLIENT_MASTER,0644);
    int IDS_clients =my_semget_create(1,PROJ_ID_CLIENT);//PROJ_ID_CLIENT est definié dans utils.h
    int IDS_master = my_semget_create(0,PROJ_ID_MASTER);//PROJ_ID_MASTER  est définié dans utils.h

    //END TODO
    loop(&data);

    //TODO destruction des tubes nommés, des sémaphores, ...
    ret= unlink(MASTER_CLIENT);
    assert(ret == 0 ) ;
    ret= unlink(CLIENT_MASTER);
    assert(ret == 0 );
    
    ret = semctl(IDS_clients, -1, IPC_RMID);
    assert(ret != -1);
    ret = semctl(IDS_master, -1, IPC_RMID);
    assert(ret != -1);
    
    TRACE0("[master] terminaison\n");
    return EXIT_SUCCESS;
}
