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

/* deklarace glob�ln�ch prom�nn�ch
  jejich pou�it� nen� ide�ln� av�ak bylo pou�ito v r�mci urychlen� pr�ce,
   nebot nejsou pot�eba p�ed�vat jako parametry jednotliv�m fc�m */

int zidli=0, cas_zakazniku=0, cas_strihani=0, zakazniku=0, cislo_ulohy=0,
 cislo_zakaznika=0, pocet_mist=0, pocet_hotovo=0;
int *cislo_ulohy2, *cislo_zakaznika2, *pocet_mist2, *pocet_hotovo2;
char *soubor_tisk_nazev;
FILE *soubor_tisk;
pid_t pid1, pid;
sem_t *xhrivn01_VSTUP, *xhrivn01_SEDIM, *xhrivn01_VSTAN,
    *xhrivn01_PRICHOZI, *xhrivn01_PAMET, *xhrivn01_DOKONCENO;

/************************** FUNK�N� PROTOTYPY ******************/
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
  } //tisk n�pov�dy
  zpracuj_param(argc,argv);
  if ((strcmp("-", soubor_tisk_nazev)!=0)){
      soubor_tisk=fopen(soubor_tisk_nazev,"w");
      if (soubor_tisk==NULL){chyba(CHYBA_OTEVRENI_SOUBORU);}
      setbuf(soubor_tisk, NULL);
  }else{
    soubor_tisk=stdout;
  }
    /********************* Sd�len� pam� ***********************/
/*************** ��slo �lohy ********************/
//zde se inkrementuje ��slo tisknuteln� operace
  if ((cislo_ulohy=shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666))==-1){
    chyba(SDIL_PAMET);
  }
  if((cislo_ulohy2=(int*)shmat(cislo_ulohy, NULL, 0)) == (void *) -1){
    chyba(SDIL_PAMET);
  }
  *cislo_ulohy2=0;

/*************** ��slo z�kazn�ka ********************/
//jednozna�n� identifik�tor ka�d�ho z�kazn�ka
  if((cislo_zakaznika=shmget(IPC_PRIVATE, sizeof(int),IPC_CREAT | 0666))==-1){
    chyba(SDIL_PAMET);
  }
  if((cislo_zakaznika2=(int*)shmat(cislo_zakaznika, NULL, 0)) == (void *) -1){
    smaz_pam(1); chyba(SDIL_PAMET);
  }
  *cislo_zakaznika2=0;
/*************** voln�ch m�st v �ek�rn� ********************/
  if ((pocet_mist=shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666))==-1){
    chyba(SDIL_PAMET);
  }
  if((pocet_mist2=(int*)shmat(pocet_mist, NULL, 0)) == (void *) -1){
    smaz_pam(2); chyba(SDIL_PAMET);
  }
  *pocet_mist2=zidli;
/*************** po�et obslou�en�ch z�kazn�k� ********************/
  if ((pocet_hotovo=shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666))==-1){
    chyba(SDIL_PAMET);
  }
  if((pocet_hotovo2=(int*)shmat(pocet_hotovo, NULL, 0)) == (void *) -1){
    smaz_pam(3); chyba(SDIL_PAMET);
  }
  *pocet_hotovo2=0;

    /********************* Sd�len� pam� konec ***********************/

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

/************** konec semafor�!!! ************/

pid_t id_zakazniku[zakazniku];
for (int i=0; i<zakazniku; i++){id_zakazniku[i]=0;}

  pid1=fork(); //vytvo�en� holi�e
  if (pid1==0){
    holic();
  }else if(pid1>0){
    //p�vodn� program
    useconds_t cekani;
    for (int i=0;i<zakazniku;i++){ //vytv��en� z�kazn�k�
      if (cas_zakazniku>0){
        cekani = random()%cas_zakazniku*1000;
        usleep(cekani);
      }
      pid=fork(); //vytvo�en� z�kazn�ka
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
  }else if (pid1<0){ //chyba p�i vytv��en� holi�e
    smaz_pam(4);
    smaz_sem(6);
    chyba(FORK);
  }

  for (int i=0; i<zakazniku; i++){waitpid(id_zakazniku[i],NULL,0);}
  //�ek�me na ukon�en� z�kazn�k�
  waitpid(pid1, NULL, 0); //�ek�me na ukon�en� procesu holi�e

  if ((strcmp("-", soubor_tisk_nazev)!=0)){ fclose(soubor_tisk); }
  smaz_pam(4);
  smaz_sem(6);

  return EXIT_SUCCESS;
}
/******************** END MAIN *****************************************/

/**
Fce vykon�v� ve�kerou �inost o�ek�vanou od z�kazn�ka holi�stv�.
od p��chodu do �ek�ry, zkontrolov�n� po�tu voln�ch m�st,
 posazen� se �i odchod na�tv�n
a� po vstup do �ek�rny, nech�n� se ost��hat, pod�kov�n� a
 odchod (ukon�en� procesu).
o v�ech sv�ch �innostech informuje v�pisem.
Ka�d� z�kazn�k je ozna�en jedine�n�m ��slem
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
        (*pocet_mist2)--; //jsme v �ek�rn�-sn��me po�et voln�ch m�st

    if (*pocet_mist2<0){/******* REFUSED ********/
          (*cislo_ulohy2)++;
            fprintf(soubor_tisk,"%d: customer %d: refused\n",
                    *cislo_ulohy2, cislo_zakaznika_tisk);
      (*pocet_mist2)++;
      // nem�me m�sto tak zv���me po�et vol. m�st v �ek�rn� zp�t na 0
      (*pocet_hotovo2)++;
       //p�id�me se do "seznamu" ji� obslou�en�ch z�kazn�k�
      if ((*pocet_hotovo2)==zakazniku){kill(pid1, SIGKILL);}
      //jsme posledn� z�kazn�k, holi� nem� na koho �ekat tak ho zabijeme
      if (sem_post(xhrivn01_PAMET)==-1){
        smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
      shmctl(cislo_ulohy, IPC_RMID, NULL);
      shmctl(cislo_zakaznika, IPC_RMID, NULL);
      shmctl(pocet_mist, IPC_RMID, NULL);
      shmctl(pocet_hotovo, IPC_RMID, NULL);
      exit(EXIT_SUCCESS);
    }
    if (zidli==volnych_zidli){ //jsem jedin� v �ek�rn�->holi� sp�
    if (sem_post(xhrivn01_PRICHOZI)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);} //vzbud�me holi�e
  }
  if (sem_post(xhrivn01_PAMET)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}

  if (sem_wait(xhrivn01_VSTUP)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);} //m��u vstoupit do k�esla
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
     //d�me v�d�t holi�ovi �e m��e za��t
  if (sem_wait(xhrivn01_VSTAN)==-1){
    smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    //�ek�me a� m��eme vst�t
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
Holi� kontroluje �ek�rnu, pokud je pr�zdn� �ek� a� ho n�kdo vzbud�
pot� st��h� z�kazn�ka a vyhazuje ho zedve�� podost��h�n�.
o sv� �innosti informuje ve v�pisu na stdout p��p do souboru
**/
/*********************** HOLIC ******************************************/
void holic(){
  int hotovych=0;
    if (sem_wait(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    hotovych=(*pocet_hotovo2);
    if (sem_post(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}

  while(hotovych!=zakazniku){ //dokud nejsou obslou�eni v�ichni z�kazn�ci
    if (sem_wait(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
          (*cislo_ulohy2)++;
            fprintf(soubor_tisk,"%d: barber: checks\n",*cislo_ulohy2);
      int volnych_zidli=*pocet_mist2;
    if (sem_post(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
    if (volnych_zidli<zidli){ //n�kdo tam je
      strihani();
    }else{//nikdo v �ek�rn�-jdi sp�t
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
Fce je "vytr�ena" z fce holi�e kde by se zbyte�n� opakovala
ovl�daj� se v n� semafory spojen� se st��h�n�m z�kazn�ka holi�em
Jeliko� fce pat�� holi�i vypisuje o sv� �innosti informace jako holi�
**/
/********************** ST��H�N� **********************/
void strihani(){
    if (sem_wait(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
        (*cislo_ulohy2)++;
          fprintf(soubor_tisk,"%d: barber: ready\n",*cislo_ulohy2);
    if (sem_post(xhrivn01_PAMET)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}

    if (sem_post(xhrivn01_VSTUP)==-1){
      smaz_pam(4); smaz_sem(6); chyba(PRACE_SEM);}
      //uvoln�me vstup pro z�kazn�ka
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
Fce na zpracov�n� parametr�.
Spr�vn� parametry jsou 3 nez�porn� ��sla,
 pot� jedno kladn� a n�zev souboru �i znak "-"
v opa�n�m p��pad� se vypisuje chybov� hl�en�
v p��pad� spr�vn�ch parametr� jsou informace ukl�d�ny do glob�ln�ch prom�nn�ch
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
Fce vypisuje chybov� hl�en� podle ��sla kter� dostane
 a ukon�uje program s chybov�m hl�en�m
p�d jej�m zavol�n�m mus� b�t odstran�ny v�echny naalokovan� prost�edky
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
Fce p�i zachycen� sign�lu ukon�en� ukl�d� sd�len�
zdroje a ukon�� se, co� se roze�le i v�em p��padn�m potomk�m
**/
void uklid(){
  smaz_pam(4);
  smaz_sem(6);
  exit(EXIT_FAILURE);
}

/**
Fce kter� ma�e semafory
parametrem se j� zad�v� po�et semafor� kter� se maj� smazat
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
Fce kter� ma�e naalokovanou sd�lenou pam�t
parametrem se j� zad�v� po�et prom�nn�ch kter� se maj� uvolnit
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
jednoduc� fce tisknouc� n�pov�du k programu
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
