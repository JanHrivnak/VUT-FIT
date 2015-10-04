#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#DKA:xhrivn01

# Skript na determinizaci konecneho automatu
# Autor: Jan Hrivnak - xhrivn01@stud-fit.vutbr.cz
# 4/2012

import getopt #parametry
import sys #stdin, stdout, stderr
from tridy import *
from funkce import *

#spoustet s :    export LC_ALL=cs_CZ.UTF-8;

#########################################  MAIN  #######################################################
if __name__ == "__main__":

    #zpracujeme zadane parametry
    (input, output, no_epsilon, determinizace, case_insensitiv)=zpracujParam(input=None, output=None, no_epsilon=0, determinizace=0, case_insensitiv=0)

    #vytvorime instance trid pomoci kterych budeme ukladat zadany automat
    stavy=mnozina()
    abeceda=mnozina()
    koncove_stavy=mnozina()
    prechody=trida_prechodu()

    #vytvorime instanci tridy pro cteni ze vstupniho souboru
    soubor_cteni=cti(input, case_insensitiv)
    soubor=iter(soubor_cteni)

    #pomoci iteratoru (funkce next) dostaneme jeden nacteny token
    token=next(soubor)
    if token[0]!='zavorka_kul':
        sys.stderr.write("Chybny format vstupniho souboru3")
        sys.exit(40)
    token=next(soubor)
    if token[0]!='zavorka_sloz':
        sys.stderr.write("Chybny format vstupniho souboru4")
        sys.exit(40)

    #cteni stavu
    ulozStavy(stavy, soubor)
    zacMnoziny(soubor)

    #cteni vstupni abecedy
    ulozStavy(abeceda, soubor)
    if len(abeceda.seznam)==0:
        sys.stderr.write("Chybny format vstupniho souboru-vstupni abeceda je prazdna")
        sys.exit(41)
    zacMnoziny(soubor)

    #cteni prechodu
    ulozPrechody(prechody,soubor, stavy, abeceda)

    token=next(soubor)
    if token[0]!='carka':
      sys.stderr.write("Chybny format vstupniho souboru-chybi carka mezi prechody a startovacim stavem")
      sys.exit(40)
    token=next(soubor)

    #cteni startovaciho symbolu
    if token[0]=='id':
        start=token[1]
    else:
        sys.stderr.write("Chybny format vstupniho souboru-chybi nacitany startovaci stav")
        sys.exit(40)

    token=next(soubor)
    if token[0]!='carka':
      sys.stderr.write("Chybny format vstupniho souboru-chybi carka pred koncovymi stavy")
      sys.exit(40)
    token=next(soubor)
    if token[0]!='zavorka_sloz':
      sys.stderr.write("Chybny format vstupniho souboru-chybi zavorka pred koncovymi stavy")
      sys.exit(40)

    # cteni koncovych stavu
    ulozStavy(koncove_stavy, soubor)
    
    token=next(soubor)
    if token[0]!='zavorka_kul_end':
      sys.stderr.write("Chybny format vstupniho souboru-chybi ) na konci souboru")
      sys.exit(40)

    #kontrola zda jsme na konci souboru (neni smeti za koncem automatu)
    #bile znaky tam byt mohou, lex. analyza je preskakuje
    token=next(soubor)
    if token[0]!='konec':
        sys.stderr.write("Chybny format vstupniho souboru-smeti na konci souboru")
        sys.exit(40)


########KONTROLA SPRAVNOSTI NACTENEHO VSTUPU
# vyznam techto jednotlivych kontrol je zrejmy z pripadne hlasene chyby
    for n in stavy.seznam:
        delka=len(n)
        if n[delka-1]=='_' or n[0]=='_':
            sys.stderr.write("chybny nazev identifikatoru, obsahuje _ na nedovolenem miste")
            sys.exit(40)

    for n in abeceda.seznam:
        if len(n)>1:
            sys.stderr.write("chybny znak ve vstupni abecede-znak musi byt jednoznakovy")
            sys.exit(40)

    for n in koncove_stavy.seznam:
        if n not in stavy.seznam:
            sys.stderr.write("Koncovy stav neni vyjmenovan v zadani stavu automatu")
            sys.exit(41)
    for n in stavy.seznam:
        if n in abeceda.seznam:
            sys.stderr.write("Vstupni abeceda neni disjunktni se stavy automatu")
            sys.exit(41)

    if start not in stavy.seznam:
        sys.stderr.write("Startovaci stav neni vyjmenovan ve stavech automatu")
        sys.exit(41)

    #tiskni(stavy, abeceda, prechody, start, koncove_stavy) #tiskne vstup

    # nastaveni toho, kam se bude tisknout vystup
    if output!= "stdout":
        stdout_old=sys.stdout
        f_output=open(output, mode='w', encoding='utf-8')
        sys.stdout=f_output

    if no_epsilon==1 or determinizace==1:
        for n in stavy.seznam:
            #spocitame epsilon uzaver pro vsechny stavy
            uzaver=prechody.eps_uzaver(n)

            # a provedeme odstraneni epsilon prechodu
            prechody.no_eps(n, uzaver)

        if "''" in abeceda.seznam:
            abeceda.seznam.remove("''")

        #print("----------------")
        # #for i in stavy.seznam:
        #    if i in prechody.prechod:
        #        print(i,": ", prechody.prechod[i])
        #print("----------------")

        if determinizace==1:
            #vytvorime node instance trid pro ulozeni vysledneho automatu
            new_stavy=mnozina()
            new_abeceda=mnozina()
            new_koncove_stavy=mnozina()
            vysledek=trida_prechodu()

            # funkci predame vstupni KA
            # na vystupu dostaneme zdeterminizovanou podobu
            (new_stavy, vysledek, new_koncove_stavy)=prechody.determinizace(stavy, abeceda, start, koncove_stavy)

            #print(vysledek)
            tiskni(new_stavy, abeceda, vysledek, start, new_koncove_stavy)
        else:
            tiskni(stavy, abeceda, prechody, start, koncove_stavy)
    else:
        tiskni(stavy, abeceda, prechody, start, koncove_stavy)

    #uzavreni vystupniho souboru
    if output!= "stdout":
        f_output.close
        sys.stdout=stdout_old

### konec souboru dka.py