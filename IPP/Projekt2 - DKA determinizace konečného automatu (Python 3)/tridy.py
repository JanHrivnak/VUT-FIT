# -*- coding: utf-8 -*-
from base64 import decode
import sys #stdin, stdout, stderr

######### CTI
# funkce pro nacitani ze vstupniho souboru
# je implementovana jako iterator, vracejici jeden nacteny token i s jeho obsahem (např. nazev identifikatoru)
class cti:
    def __init__(self, input, case_insensitiv):  #string do ktereho nacteme cely vstupni KA, bud ze souboru nebo ze stdin
        self.case_insensitiv=case_insensitiv
        if input!= "stdin":
            try:
                f_input = open(input, encoding='utf-8')
            except:
                sys.stderr.write("Zadany soubor neexistuje, nebo nelze otevrit")
                sys.exit(1)
            self.vstupni_text=f_input.read()
            f_input.close()
        else:
            self.vstupni_text = sys.stdin.read()
        self.delka=len(self.vstupni_text)
        #print(self.vstupni_text)
        #print(self.delka)

    def __iter__(self):
        self.pozice=0
        return self

    #samotna lexikalni analyza
    #vraci nacteny token a jeho obsah
    def __next__(self):
        identifikator=None
        stav='init'
        while True:
            #jsme na konci souboru
            if self.delka == self.pozice:
                self.pozice+=1
                if stav=='sipka' or stav=='text' or stav=='text3' or stav=='text4':
                    sys.stderr.write("Neocekavany konec vstupniho souboru")
                    sys.exit(40)
                elif stav=='identifikator':
                    if self.case_insensitiv==1:
                        identifikator=identifikator.lower()
                    return('id', identifikator)
                elif stav=='text2':
                    return('nic', '')
            if self.delka <= self.pozice:
                return('konec', '')

            znak=self.vstupni_text[self.pozice] #read
            self.pozice+=1                      #posunuti adresy pristiho cteni
            if stav=='init':
                if znak=='(':
                    return('zavorka_kul', znak)
                elif znak==')':
                    return('zavorka_kul_end', znak)
                elif znak=='{':
                    return('zavorka_sloz', znak)
                elif znak=='}':
                    return('zavorka_sloz_end', znak)
                elif znak==',':
                    return('carka', znak)
                elif znak=='#':
                    stav='komentar'
                elif znak=='-':
                    stav='sipka'
                elif znak=='\'':
                    stav='text'
                elif str.isspace(znak):
                    stav='init'
                elif str.isalpha(znak) or znak=='_':
                    identifikator=znak
                    stav='identifikator'
                else:
                    return('nevim', znak)

            elif stav=='komentar':
                if znak=='\n':
                    stav='init'

            elif stav=='sipka':
                if znak=='>':
                    return('sipka', '->')
                else:
                    sys.stderr.write("Chybny vstup -?")
                    sys.exit(40)

            elif stav=='text':
                if znak=='\'':
                    stav='text2'
                else:
                    identifikator=znak
                    stav='text4'
            elif stav=='text2':
                if znak=='\'':
                    stav='text3'
                else:
                    return('nic', '')
            elif stav=='text3':
                if znak=='\'':
                    return('id', '\'')
                else:
                    sys.stderr.write("Chybny vstup '''?")
                    sys.exit(40)
            elif stav=='text4':
                if znak=='\'':
                    if self.case_insensitiv==1:
                        identifikator=identifikator.lower()
                    return('id', identifikator)
                else:
                    stav='text4'
                    identifikator+=znak
            elif stav=='identifikator':
                if str.isalpha(znak) or str.isdigit(znak) or znak=='_':
                    identifikator+=znak
                else:
                    self.pozice-=1
                    if self.case_insensitiv==1:
                        identifikator=identifikator.lower()
                    return('id', identifikator)


########## TABULKA PRECHODU
# trida pro ukladani prechodu automatu
class trida_prechodu:
    def __init__(self):
        self.prechod={}

    # fce ulozi prechod ktery dostane na vstupu ve spravnem formatu
    def pridej(self, stav, symbol, vystup):
        radek={}
        #kontrola jestli tento stav jiz nejaky prechod ma
        if stav not in self.prechod:
            self.prechod[stav]=None
            radek={}
        else:
            radek=self.prechod[stav]
        if symbol not in radek:
            radek[symbol]=[vystup,]
        else:
            prom=radek[symbol]
            if vystup not in prom:
              prom.append(vystup)
              radek[symbol]=prom
        self.prechod[stav]=radek
        #print(stav, self.prechod[stav])
        return (self)

    def vypis_stav(self, stav):
        print (stav, self.prechod[stav])

    def vypis_prechod(self, stav, symbol):
        print (self.prechod[stav][symbol])

    #funkce pro vypocet epsilon uzaveru predaneho stavu
    # vraci seznam stavu, ktere jsou v tomto uzaveru
    def eps_uzaver(self, stav):
        uzaver=[]
        if stav not in self.prechod:  # u stavu existuji nejake prechody
            return(None)
        if "''" in self.prechod[stav].keys():  # u stavu existuje Epsilon prechod
            uzaver = [i for i in set(self.prechod[stav]["''"])]
            if stav in uzaver:
                uzaver.remove(stav)
            new_uzaver=[]
            for n in uzaver:
                if n != stav:
                    vysl=self.eps_uzaver(n)
                    if vysl != None:
                        for x in vysl:
                            new_uzaver.append(x)
            if new_uzaver!=[]:
                for x in new_uzaver:
                    uzaver.append(x)
            return(uzaver)
        else:
            return (None)

    # funkce pro odstraneni epsilon prechodu
    # vstup: stav pro ktery odstranujeme eps prechody a epsilon uzaver
    def no_eps(self, stav, uzaver):
        if stav in self.prechod:
            #print("prechody ",stav," pred: ", self.prechod[stav])
            if "''" in self.prechod[stav].keys():
                del(self.prechod[stav]["''"])
            if uzaver != None: #epsilon uzaver neni prazdny
                for stav_z_uzaveru in uzaver:
                    if stav_z_uzaveru in self.prechod: #pro stav, z eps uzaveru neexistuje zadny prechod, nemuseme pro neho nic pocitat
                        for znak in self.prechod[stav_z_uzaveru]:
                            if znak != "''": # epsilon prechody neresime, ty jsou jiz vyreseny epsilon uzaverem
                                #self.prechod[stav].append(m)
                                for cil in self.prechod[stav_z_uzaveru][znak]:
                                    self.pridej(stav, znak, cil)
        else:  #pro znak nejsou zadne prechody
            return(None)

###### DETERMINIZACE
    #funkce pro samotnou determinizaci automatu
    # jeji cinnost je popsana v dokumentaci
    # na vstupu dostava vsechny potrebne vstupni casti automatu
    # na vystupu vraci nove objekty s vystupnim deterministickym automatem
    def determinizace(self, stavy, abeceda, start, koncove_stavy):

        new_stavy=mnozina()
        new_koncove_stavy=mnozina()
        vysledek=trida_prechodu()
        
        new_stavy.pridej(start)
        if start in koncove_stavy.seznam:
            new_koncove_stavy.pridej(start)
        all=[] #seznam vsech stavu automatu ktere se budou prochazet
        all.append(start)
        all_podstavy={} #vsechny stavy ze kterych je vygenerovan jeden, spojeny podtrzitky
        all_podstavy[start]=all
        
        for stav in all:  # pro vsechny vygenerovane stavy (s, f_q1, f_q2)
            for znak in abeceda.seznam:  #pro kazdy znak ze vstupni abecedy (a, b, c)
                in_stavy=[] #zde se ukladaji stavy ze kterych se bude dohromady generovat novy uzel
                koncovy=0 # poznamka zda novy generovany uzel bude koncovy
                pocet_stavu=0
                #print("ALL-",all_podstavy)
                for puv_stav in all_podstavy[stav]: # pro vsechny stavy ze kterych se tento generovany sklada (f, q1)
                    #print (stav, znak, puv_stav)
                    if puv_stav in self.prechod and znak in self.prechod[puv_stav]:
                        for cil in self.prechod[puv_stav][znak]:  #pro f,g  z prechodu s'a ->[f, g]
                            if cil in koncove_stavy.seznam:
                                koncovy=1
                            if cil not in in_stavy:
                                in_stavy.append(cil)
                            pocet_stavu+=1
                #print(in_stavy)
                if pocet_stavu>0:  #else pro tento stav neni zadny vystup -> v prapade dalsi implementace rozsireni se zde prida vystup do Qfalse
                    #vygenerovat nazev a ulozit
                    nazev=""
                    in_stavy.sort()
                    for x in in_stavy:
                        nazev+="_"+x
                    nazev=nazev[1:] #i prvni stav mame ulozen s podtrzitkem na zacatku tak ho musime odstanit
                    vysledek.pridej(stav, znak, nazev)
                    if nazev not in all: #jiz existujici stavy znovu nepridavame
                        all.append(nazev)
                        all_podstavy[nazev]=in_stavy
                        new_stavy.pridej(nazev)
                        if koncovy==1:
                            new_koncove_stavy.pridej(nazev)
        return (new_stavy, vysledek, new_koncove_stavy)

    # funkce rpo vypsani prechodu v normovanem formatu
    # funkce postupne radi vstupni symboly, pote znaky vstupni abecedy
    def vypis(self):
        vystup=""
        stavy=[]
        for x in self.prechod:
            stavy.append(x)
        stavy.sort() #seradime stavy

        for i in stavy: #prochazime serazenymi stavy
            serazeneZnaky=[]
            for x in self.prechod[i]:
                serazeneZnaky.append(x)
            serazeneZnaky.sort()

            for j in serazeneZnaky:
                serazeneCile=[]
                for x in self.prechod[i][j]:
                    serazeneCile.append(x)
                serazeneCile.sort()

                if j=='\'':
                    j='\'\''
                if j=='':
                    j="''"
                else:
                    j="\'"+j+"\'"

                for k in serazeneCile:
                    vystup+=(i+" "+j+" -> "+k+",\n")
        delka=len(vystup)
        print(vystup[0:delka-2], end="")
        if len(vystup)>1:
            print("\n", end="")



######### MNOZINA
# trida implementujici obycejny seznam a nad nim funkce pro pridani prvku (brani duplicitam)
# dalsi je funkce na vypis ve formatu pozadovanem v zadani
class mnozina:
    def __init__(self):
        self.seznam=[]

    def pridej(self, prom):
        if prom not in self.seznam:
            self.seznam.append(prom)

    def vypis(self, apostrofy):
        text=""
        self.seznam.sort()
        for n in self.seznam:
            if n=='\'':
                n='\'\''
            if apostrofy==1:
                text+=("'"+n+"', ")
            else:
                text+=(n+", ")
        delka=len(text)
        print(text[0:delka-2], end="")
