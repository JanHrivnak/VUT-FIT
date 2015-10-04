/** Autor: Jan Hrivnak
    Datum vytvoreni: konec dubna 2011
    Program: cilem programu je implementace synchronizacniho problemu
     spiciho holice pomoci semaforu
    program se spusti s 10 zidlemi v cekarne,
     nulovymi zpozdenimy a 1 zakaznikem
**/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sched.h>

#define SDIL_PAMET 1
#define FORK 2
#define CHYBA_PARAMETRY 3
#define CHYBA_OTEVRENI_SOUBORU 4
#define MAKE_SEM 5
#define DEL_SEM 6
#define PRACE_SEM 7

/* deklarace globálních promìnných
  jejich pou¾ití není ideální av¹ak bylo pou¾ito v rámci urychlení práce,
   nebot nejsou potøeba pøedávat jako parametry jednotlivým fcím */

int zidli=0, cas_zakazniku=0, cas_strihani=0, zakazniku=0, cislo_ulohy=0,
 cislo_zakaznika=0, pocet_mist=0, pocet_hotovo=0;
int *cislo_ulohy2, *cislo_zakaznika2, *pocet_mist2, *pocet_hotovo2;
char *soubor_tisk_nazev;
FILE *soubor_tisk;
pid_t pid1, pid;
sem_t *xhrivn01_VSTUP, *xhrivn01_SEDIM, *xhrivn01_VSTAN,
    *xhrivn01_PRICHOZI, *xhrivn01_PAMET, *xhrivn01_DOKONCENO;

/************************** FUNKÈNÍ PROTOTYPY ******************/
void holic();
void strihani();
void zakaznik();
void zpracuj_param(int argc, char *argv[]);
FILE *otevri_soubor(char *nazev);
void chyba(int cislo);
void smaz_sem(int pocet);
void smaz_pam(int pocet);
void napoveda();
void uklid();

/***************** MAIN *****************************/

int main(int argc, char *argv[])
{
  signal(SIGINT, uklid);
  signal(SIGKILL, uklid);

  if ((argc==2) && (strcmp("-h", argv[1])==0)){
      napoveda();exit(EXIT_SUCCESS);
  } //tisk nápovìdy
  zpracuj_param(argc,argv);
  if ((strcmp("-", soubor_tisk_nazev)!=0)){
      soubor_tisk=fopen(soubor_tisk_nazev,"w");
      if (soubor_tisk==NULL){chyba(CHYBA_OTEVRENI_SOUBORU);}
      setbuf(soubor_tisk, NULL);
  }else{
    soubor_tisk=stdout;
  }
    /********************* Sdílená pamì» ***********************/
/*************** èíslo úlohy ********************/
//zde se inkrementuje èíslo tisknutelné operace
  if ((cislo_ulohy=shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666))==-1){
    chyba(SDIL_PAMET);
  }
  if((cislo_ulohy2=(int*)shmat(cislo_ulohy, NULL, 0)) == (void *) -1){
    chyba(SDIL_PAMET);
  }
  *cislo_ulohy2=0;

/*************** èíslo zákazníka ********************/
//jednoznaèný identifikátor ka¾dého zákazníka
  if((cislo_zakaznika=shmget(IPC_PRIVATE, sizeof(int),IPC_CREAT | 0666))==-1){
    chyba(SDIL_PAMET);
  }
  if((cislo_zakaznika2=(int*)shmat(cislo_zakaznika, NULL, 0)) == (void *) -1){
    smaz_pam(1); chyba(SDIL_PAMET);
  }
  *cislo_zakaznika2=0;
/*************** volných míst v èekárnì ********************/
  if ((pocet_mist=shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666))==-1){
    chyba(SDIL_PAMET);
  }
  if((pocet_mist2=(int*)shmat(pocet_mist, NULL, 0)) == (void *) -1){
    smaz_pam(2); chyba(SDIL_PAMET);
  }
  *pocet_mist2=zidli;
/*************** poèet obslou¾ených zákazníkù ********************/
  if ((pocet_hotovo=shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666))==-1){
    chyba(SDIL_PAMET);
  }
  if((pocet_hotovo2=(int*)shmat(pocet_hotovo, NULL, 0)) == (void *) -1){
    smaz_pam(3); chyba(SDIL_PAMET);
  }
  *pocet_hotovo2=0;

    /********************* Sdílená pamì» konec ***********************/

/************** semafory ************/

  xhrivn01_VSTUP = (sem_t *)mmap(0, sizeof(sem_t),
           PROT_READ | PROT_WRITE,MAP_ANON | MAP_SHARED, -1, 0);
  if((sem_init(xhrivn01_VSTUP, 1, 0))==-1){
    smaz_pam(4);  chyba(MAKE_SEM);
  }

  xhrivn01_SEDIM = (sem_t *)mmap(0, sizeof(sem_t),
           PROT_READ | PROT_WRITE,MAP_ANON | MAP_SHARED, -1, 0);
  if((sem_init(xhrivn01_SEDIM, 1, 0))==-1){
    smaz_pam(4);  smaz_sem(1);  chyba(MAKE_SEM);
  }

  xhrivn01_VSTAN = (sem_t *)mmap(0, sizeof(sem_t),
            PROT_READ | PROT_WRITE,MAP_ANON | MAP_SHARED, -1, 0);
  if((sem_init(xhrivn01_VSTAN, 1, 0))==-1){
    smaz_pam(4);  smaz_sem(2);  chyba(MAKE_SEM);
  }

  xhrivn01_PRICHOZI = (sem_t *)mmap(0, sizeof(sem_t),
            PROT_READ | PROT_WRITE,MAP_ANON | MAP_SHARED, -1, 0);
  if((sem_init(xhrivn01_PRICHOZI, 1, 0))==-1){
    smaz_pam(4);  smaz_sem(3);  chyba(MAKE_SEM);
  }

  xhrivn01_PAMET = (sem_t *)mmap(0, sizeof(sem_t),
            PROT_READ | PROT_WRITE,MAP_ANON | MAP_SHARED, -1, 0);
  if((sem_init(xhrivn01_PAMET, 1, 1))==-1){
    smaz_pam(4);  smaz_sem(4);  chyba(MAKE_SEM);
  }

  xhrivn01_DOKONCENO = (sem_t *)mmap(0, sizeof(sem_t),
            PROT_READ | PROT_WRITE,MAP_ANON | MAP_SHARED, -1, 0);
  if((sem_init(xhrivn01_DOKONCENO, 1, 0))==-1){
    smaz_pam(4);  smaz_sem(5);  chyba(MAKE_SEM);
  }

/************** konec semaforù!!! ************/

pid_t id_zakazniku[zakazniku];
for (int i=0; i<zakazniku; i++){id_zakazniku[i]=0;}

  pid1=fork(); //vytvoøení holièe
  if (pid1==0){
    holic();
  }else if(pid1>0){
    //pùvodní program
    useconds_t cekani;
    for (int i=0;i<zakazniku;i++){ //vytváøení zákazníkù
      if (cas_zakazniku>0){
        cekani = random()%cas_zakazniku*1000;
        usleep(cekani);
      }
      pid=fork(); //vytvoøení zákazníka
      if (pid==0){
        zakaznik();
      }else if(pid<0){ //chyba
        kill(pid1, SIGKILL);
        for (int j=0; j<i;i++){kill(id_zakazniku[j], SIGKILL);}
        smaz_pam(4);
        smaz_sem(6);
        chyba(FORK);
      }else if (pid>0){
        id_zakazniku[i]=pid;
      }
    }
  }else if (pid1<0){ //chyba pøi vytváøení holièe
    smaz_pam(4);
    smaz_sem(6);
    chyba(FORK);
  }

  for (int i=0; i<zakazniku; i++){waitpid(id_zakazniku[i],NULL,0);}
  //èekáme na ukonèení zákazníkù
  waitpid(pid1, NULL, 0); //èekáme na ukonèení procesu holièe

  if ((strcmp("-", soubor_tisk_nazev)!=0)){ fclose(soubor_tisk); }
  smaz_pam(4);
  smaz_sem(6);

  return EXIT_SUCCESS;
}
/******************** END MAIN *****************************************/

/**
Fce vykonává ve¹kerou èinost oèekávanou od zákazníka holièství.
od pøíchodu do èekáry, zkontrolování poètu volných míst,
 posazení se èi odchod na¹tván
a¾ po vstup do èekárny, nechání se ostøíhat, podìkování a
 odchod (ukonèení procesu).
o v¹ech svých èinnostech informuje výpisem.
Ka¾dý zákazník je oznaèen jedineèným èíslem
**/
/************************ ZAKAZNIK **************************/
void zakaznik(){
  if (sem_wait(xhrivn01_PAMET)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);
  }/***** CREAT ***/
    (*cislo_zakaznika2)++;
    int cislo_zakaznika_tisk=(*cislo_zakaznika2);
      (*cislo_ulohy2)++;
        fprintf(soubor_tisk,"%d: customer %d: created\n",
                *cislo_ulohy2,  cislo_zakaznika_tisk);
  if (sem_post(xhrivn01_PAMET)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}

  if (sem_wait(xhrivn01_PAMET)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}/*** ENTER ****/
      (*cislo_ulohy2)++;
        fprintf(soubor_tisk,"%d: customer %d: enters\n",
                *cislo_ulohy2,  cislo_zakaznika_tisk);
      int volnych_zidli=*pocet_mist2;
        (*pocet_mist2)--; //jsme v èekárnì-sní¾íme poèet volných míst

    if (*pocet_mist2<0){/******* REFUSED ********/
          (*cislo_ulohy2)++;
            fprintf(soubor_tisk,"%d: customer %d: refused\n",
                    *cislo_ulohy2, cislo_zakaznika_tisk);
      (*pocet_mist2)++;
      // nemáme místo tak zvý¹íme poèet vol. míst v èekárnì zpìt na 0
      (*pocet_hotovo2)++;
       //pøidáme se do "seznamu" ji¾ obslou¾ených zákazníkù
      if ((*pocet_hotovo2)==zakazniku){kill(pid1, SIGKILL);}
      //jsme poslední zákazník, holiè nemá na koho èekat tak ho zabijeme
      if (sem_post(xhrivn01_PAMET)==-1){
        smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
      shmctl(cislo_ulohy, IPC_RMID, NULL);
      shmctl(cislo_zakaznika, IPC_RMID, NULL);
      shmctl(pocet_mist, IPC_RMID, NULL);
      shmctl(pocet_hotovo, IPC_RMID, NULL);
      exit(EXIT_SUCCESS);
    }
    if (zidli==volnych_zidli){ //jsem jediný v èekárnì->holiè spí
    if (sem_post(xhrivn01_PRICHOZI)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);} //vzbudíme holièe
  }
  if (sem_post(xhrivn01_PAMET)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}

  if (sem_wait(xhrivn01_VSTUP)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);} //mù¾u vstoupit do køesla
        if (sem_wait(xhrivn01_PAMET)==-1){
          smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
        (*pocet_mist2)++;
        (*cislo_ulohy2)++;
          fprintf(soubor_tisk,"%d: customer %d: ready\n",
                  *cislo_ulohy2, cislo_zakaznika_tisk);
      if (sem_post(xhrivn01_PAMET)==-1){
        smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
  if (sem_post(xhrivn01_SEDIM)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
     //dáme vìdìt holièovi ¾e mù¾e zaèít
  if (sem_wait(xhrivn01_VSTAN)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    //èekáme a¾ mù¾eme vstát
      if (sem_wait(xhrivn01_PAMET)==-1){
        smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
        (*cislo_ulohy2)++;
          fprintf(soubor_tisk,"%d: customer %d: served\n",
                  *cislo_ulohy2, cislo_zakaznika_tisk);
      if (sem_post(xhrivn01_PAMET)==-1){
        smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
      if (sem_post(xhrivn01_DOKONCENO)==-1){
        smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
      if ((strcmp("-", soubor_tisk_nazev)!=0)){
         fclose(soubor_tisk); }

  shmctl(cislo_ulohy, IPC_RMID, NULL);
  shmctl(cislo_zakaznika, IPC_RMID, NULL);
  shmctl(pocet_mist, IPC_RMID, NULL);
  shmctl(pocet_hotovo, IPC_RMID, NULL);

  exit(EXIT_SUCCESS);
}

/**
Holiè kontroluje èekárnu, pokud je prázdná èeká a¾ ho nìkdo vzbudí
poté støíhá zákazníka a vyhazuje ho zedveøí podostøíhání.
o své èinnosti informuje ve výpisu na stdout pøíp do souboru
**/
/*********************** HOLIC ******************************************/
void holic(){
  int hotovych=0;
    if (sem_wait(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    hotovych=(*pocet_hotovo2);
    if (sem_post(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}

  while(hotovych!=zakazniku){ //dokud nejsou obslou¾eni v¹ichni zákazníci
    if (sem_wait(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
          (*cislo_ulohy2)++;
            fprintf(soubor_tisk,"%d: barber: checks\n",*cislo_ulohy2);
      int volnych_zidli=*pocet_mist2;
    if (sem_post(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    if (volnych_zidli<zidli){ //nìkdo tam je
      strihani();
    }else{//nikdo v èekárnì-jdi spát
      if (sem_wait(xhrivn01_PRICHOZI)==-1){
        smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
      strihani();
    }
    if (sem_wait(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    (*pocet_hotovo2)++;
    hotovych=(*pocet_hotovo2);
    if (sem_post(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    if (sem_wait(xhrivn01_DOKONCENO)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
  }
  if ((strcmp("-", soubor_tisk_nazev)!=0)){ fclose(soubor_tisk); }

  shmctl(cislo_ulohy, IPC_RMID, NULL);
  shmctl(cislo_zakaznika, IPC_RMID, NULL);
  shmctl(pocet_mist, IPC_RMID, NULL);
  shmctl(pocet_hotovo, IPC_RMID, NULL);
  exit(EXIT_SUCCESS);
}

/**
Fce je "vytr¾ena" z fce holièe kde by se zbyteènì opakovala
ovládají se v ní semafory spojené se støíháním zákazníka holièem
Jeliko¾ fce patøí holièi vypisuje o své èinnosti informace jako holiè
**/
/********************** STØÍHÁNÍ **********************/
void strihani(){
    if (sem_wait(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
        (*cislo_ulohy2)++;
          fprintf(soubor_tisk,"%d: barber: ready\n",*cislo_ulohy2);
    if (sem_post(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}

    if (sem_post(xhrivn01_VSTUP)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
      //uvolníme vstup pro zákazníka
    if (sem_wait(xhrivn01_SEDIM)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    useconds_t cekani2;
    if (cas_strihani>0){
      cekani2 = random()%cas_strihani*1000;
      usleep(cekani2);
    }
      if (sem_wait(xhrivn01_PAMET)==-1){
        smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
        (*cislo_ulohy2)++;
          fprintf(soubor_tisk,"%d: barber: finished\n",*cislo_ulohy2);
      if (sem_post(xhrivn01_PAMET)==-1){
        smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    if (sem_post(xhrivn01_VSTAN)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
}

/**
Fce na zpracování parametrù.
Správné parametry jsou 3 nezáporná èísla,
 poté jedno kladné a název souboru èi znak "-"
v opaèném pøípadì se vypisuje chybové hlá¹ení
v pøípadì správných parametrù jsou informace ukládány do globálních promìnných
**/
/********************* ZPRACUJ PARAM *************************/
void zpracuj_param(int argc, char *argv[]){
  if (argc!=6){
    chyba(CHYBA_PARAMETRY);
  }else{

    if(isdigit(*argv[1])){
      zidli=atoi(argv[1]);
      if (zidli<0){chyba(CHYBA_PARAMETRY);}
    }else{
      chyba(CHYBA_PARAMETRY);
    }
    if(isdigit(*argv[2])){
      cas_zakazniku=atoi(argv[2]);
      if (cas_zakazniku<0){chyba(CHYBA_PARAMETRY);}
    }else{
      chyba(CHYBA_PARAMETRY);
    }
    if(isdigit(*argv[3])){
      cas_strihani=atoi(argv[3]);
      if (cas_strihani<0){chyba(CHYBA_PARAMETRY);}
    }else{
      chyba(CHYBA_PARAMETRY);
    }
    if(isdigit(*argv[4])){
      zakazniku=atoi(argv[4]);
      if (zakazniku<=0){chyba(CHYBA_PARAMETRY);}
    }else{
      chyba(CHYBA_PARAMETRY);
    }
    soubor_tisk_nazev=argv[5];
  }
}

/**
Fce vypisuje chybové hlá¹ení podle èísla které dostane
 a ukonèuje program s chybovým hlá¹ením
pød jejím zavoláním musí být odstranìny v¹echny naalokované prostøedky
**/
/******************* CHYBA *************************/
void chyba(int cislo){
  switch (cislo){
  case SDIL_PAMET:
   fprintf(stderr, "Doslo k chybe pri vytvareni sdilene pameti.\n");
          break;
  case FORK: fprintf(stderr, "Doslo k chybe pri vytvareni procesu.\n");
          break;
  case CHYBA_PARAMETRY: fprintf(stderr, "Byly zadany chybne parametry.\n");
          break;
  case CHYBA_OTEVRENI_SOUBORU:
   fprintf(stderr, "Nepodarilo se spravne otevrit soubor pro zapis.\n");
          break;
  case MAKE_SEM: fprintf(stderr, "Nepodarilo se spravne vytvorit semafor.\n");
          break;
  case DEL_SEM: fprintf(stderr, "Nepodarilo se spravne smazat semafor.\n");
          break;
  case PRACE_SEM: fprintf(stderr, "Chyba pri praci se semaforem.\n");
          break;
default: fprintf(stderr, "Doslo k chybe programu \n");
  }
  fprintf(stderr,"Pro zobrazeni napovedy spustte program s parametrem -h\n");
exit(EXIT_FAILURE);
}

/**
Fce pøi zachycení signálu ukonèení uklídí sdílené
zdroje a ukonèí se, co¾ se roze¹le i v¹em pøípadným potomkùm
**/
void uklid(){
  smaz_pam(4);
  smaz_sem(6);
  exit(EXIT_FAILURE);
}

/**
Fce která ma¾e semafory
parametrem se jí zadává poèet semaforù které se mají smazat
**/
void smaz_sem(int pocet){
  if(pocet>=1){
    if (sem_destroy(&(*xhrivn01_VSTUP))==-1){chyba(DEL_SEM);}
  }
  if(pocet>=2){
    if (sem_destroy(&(*xhrivn01_SEDIM))==-1){chyba(DEL_SEM);}
  }
  if(pocet>=3){
    if (sem_destroy(&(*xhrivn01_VSTAN))==-1){chyba(DEL_SEM);}
  }
  if(pocet>=4){
    if (sem_destroy(&(*xhrivn01_PRICHOZI))==-1){chyba(DEL_SEM);}
  }
  if(pocet>=5){
    if (sem_destroy(&(*xhrivn01_PAMET))==-1){chyba(DEL_SEM);}
  }
  if(pocet>=6){
    if (sem_destroy(&(*xhrivn01_DOKONCENO))==-1){chyba(DEL_SEM);}
  }
}

/**
Fce která ma¾e naalokovanou sdílenou pamìt
parametrem se jí zadává poèet promìnných které se mají uvolnit
**/
void smaz_pam(int pocet){
  if(pocet>=1){shmdt(cislo_ulohy2);
      shmctl(cislo_ulohy, IPC_RMID, NULL);}
  if(pocet>=2){shmdt(cislo_zakaznika2);
    shmctl(cislo_zakaznika, IPC_RMID, NULL);}
  if(pocet>=3){shmdt(pocet_mist2);
      shmctl(pocet_mist, IPC_RMID, NULL);}
  if(pocet>=4){shmdt(pocet_hotovo2);
      shmctl(pocet_hotovo, IPC_RMID, NULL);}
}

/**
jednoducá fce tisknoucí nápovìdu k programu
**/
void napoveda(){
  printf("Autor: Jan Hrivnak\n"
         "Datum vytvoreni: konec dubna 2011\n"
         "Program: cilem programu je implementace "
         "synchronizacniho problemu spiciho holice pomoci semaforu\n"
         "Priklad spusteni: ./barbers 10 0 0 1 -\n"
         "program se spusti s 10 zidlemi v cekarne,"
         " nulovymi zpozdenimy a 1 zakaznikem\n");
}
