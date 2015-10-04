# -*- coding: utf-8 -*-
import getopt #parametry
import sys #stdin, stdout, stderr

# Funkce slouzi ke zpracovani zadanych parametru
# vyuziva knihovnu getopt
# vstup: pouze za ucelem deklarace promennych a nastaveni jejich vychozich hodnot (pri volani)
# vystup: (input, output, no_epsilon, determinizace, case_insensitiv)
def zpracujParam(input, output, no_epsilon, determinizace, case_insensitiv):
    try:
        opts, args = getopt.getopt(sys.argv[1:], "edi", ["help", "input=", "output=", "no-epsilon-rules", "determinization", "case-insensitive"])
    except (getopt.GetoptError):
        sys.stderr.write ("Chybne zadane parametry. Pro napovedu spustte skript s parametrem --help.")
        sys.exit(1)

    for options, param_text in opts:
        if options == "--help":
            if no_epsilon!=0 or determinizace !=0 or case_insensitiv!=0 or input!=None or output!=None:
                sys.stderr.write("Parametr --help muze byt zadan pouze samostatne.")
                sys.exit(1)
            napoveda()
            sys.exit() # vytistena napoveda, koncime

        elif options in ("-e", "--no-epsilon-rules"):
            if no_epsilon==0:
                no_epsilon=1
            else:
                sys.stderr.write("Parametr -e se opakuje. Pro napovedu spustte skript s prametrem --help.")
                sys.exit(1)

        elif options in ("-d", "--determinization"):
            if determinizace==0:
                determinizace=1
            else:
                sys.stderr.write("Parametr -d se opakuje. Pro napovedu spustte skript s prametrem --help.")
                sys.exit(1)

        elif options in ("-i", "--case-insensitive"):
            if case_insensitiv==0:
                case_insensitiv=1
            else:
                sys.stderr.write("Parametr -i se opakuje. Pro napovedu spustte skript s prametrem --help.")
                sys.exit(1)

        elif options == "--input":
            if input == None:
                input=param_text
            else:
                sys.stderr.write("Parametr --input se opakuje. Pro napovedu spustte skript s prametrem --help.")
                sys.exit(1)

        elif options == "--output":
            if output == None:
                output=param_text
            else:
                sys.stderr.write("Parametr --output se opakuje. Pro napovedu spustte skript s prametrem --help.")
                sys.exit(1)
        else:
            assert False, "Neznama volba"

    if no_epsilon==1 and determinizace==1:
        sys.stderr.write("Parametry -d a -e nesmi byt zadany spolecne. Pro napovedu spustte skript s prametrem --help.")
        sys.exit(1)
    if input==None:
        input="stdin"
    if output==None:
        output="stdout"

    return (input, output, no_epsilon, determinizace, case_insensitiv)

########## NAPOVEDA
# funkce pro tisk napovedy
# nema zadne vstupni ani vystupni parametry
def napoveda():
    text="""
    IPP-proj2
    autor: Jan Hrivnak, xhrivn01@stud.fit.vutbr.cz

    pouziti skriptu:
    --help - vypise tuto napovedu
    --input - obsahuje nazev vstupniho souboru s KA. Pokud neni zadano bere se STDIN
    --output - obsahuje nazev vystupniho souboru s KA. Pokud neni zadano vypisuje se na STDOUT
    -e, --no-epsilon-rules - pouhe odstraneni epsilon prechodu
    -d, --determizization - provede determinizaci
    -i, --case-insensitiv - KA bude bran jako case insensitiv"""
    print(text)


########## CTENI STAVU
# fce pro nacteni stavu
# dostava na vstupu mnozinu do ktere bude ukladat nactene stavy a soubor ze ktereho cte
# podle formalni definice vstupu predpoklada spravne poradi identifikator+carka nebo koncova zavorka. Jinak hlasi chybu
def ulozStavy(mnozina, soubor):
    neprazdna=0
    while True:
        token=next(soubor)
        if token[0]=='id' or token[0]=='uvozovky':
            mnozina.pridej(token[1])
            neprazdna=1
        elif token[0]=='zavorka_sloz_end': #prazdna mnozina nebo ... , }
            if neprazdna==0:
                break
            else:
                sys.stderr.write("Chybny format vstupniho souboru-za carkou je }")
                sys.exit(41)
        else:
            #print(token[0])
            sys.stderr.write("Chybny format vstupniho souboru-chybi identifikator")
            sys.exit(41)
        token=next(soubor)
        if token[0]=='carka':
            continue
        elif token[0]=='zavorka_sloz_end':
            break
        else:
            sys.stderr.write("Chybny format vstupniho souboru-za identifikatorem neni , nebo }")
            sys.exit(40)

########## CTENI PRECHODU
# funkce pro cteni prechodu ve tvaru p a -> q, nebo p -> q
# jako vstupni parametr dostava instanci tridy do ktere uklada vystupy
# dalsi vstupni parametr je soubor ze ktereho se cte
# poslednimi parametry jsou mnoziny se stavy automatu a abecedou, vuci kterym se kontroluje spravnost precteneho pravidla
def ulozPrechody(prechody, soubor, stavy, abeceda):
    neprazdna=0
    while True:
        token=next(soubor)
        if token[0]=='id':
            stav=token[1]
            neprazdna=1
        elif token[0]=='zavorka_sloz_end': #zadne prechody nebo je na vstupu ", }"
            if neprazdna==0:
                break
            else:
                sys.stderr.write("Chybny format vstupniho souboru-za carkou prechodu je }")
                sys.exit(40)
        else:
            sys.stderr.write("Chybny format vstupniho souboru-chybi identifikator")
            sys.exit(40)
        sipka=0
        token=next(soubor)
        if token[0]=='id' or token[0]=='uvozovky':
            if token[1] in abeceda.seznam:
                znak=token[1]
            else:
                sys.stderr.write("Chybny format vstupniho souboru-prechod s nepovolenym znakem")
                sys.exit(40)
        elif token[0]=='sipka':
            # nena zadan cteny znak (pridame "espilon")
            sipka=1
            znak="''"
        else:
            sys.stderr.write("Chybny format vstupniho souboru-chybi nacitany prechodovy znak")
            sys.exit(40)

        if sipka==0:
          token=next(soubor)
          if token[0]!='sipka':
              sys.stderr.write("Chybny format vstupniho souboru-chybi sipka")
              sys.exit(40)

        # cilovy stav prechodu
        token=next(soubor)
        if token[0]=='id':
            cil=token[1]
        else:
            sys.stderr.write("Chybny format vstupniho souboru-chybi identifikator")
            sys.exit(40)
            
        #hlidani chyb
        if stav not in stavy.seznam:
            sys.stderr.write("Prechod ze stavu, ktery neni vyjmenovan v zadani stavu automatu")
            sys.exit(41)
        if cil not in stavy.seznam:
            sys.stderr.write("Cilovy stav prechodu neni vyjmenovan v zadani stavu automatu")
            sys.exit(41)

        prechody.pridej(stav, znak, cil)

        token=next(soubor)
        if token[0]=='carka':
            continue
        elif token[0]=='zavorka_sloz_end':
            break
        else:
            sys.stderr.write("Chybny format vstupniho souboru-za identifikatorem neni , nebo }")
            sys.exit(40)

#funkce ktera sdruzuje kod ktery by se musel rozkopirovavat
# hlida carku mezi mnozinami a zacatek slozene zavorky
# na vstupu dostava soubor ze ktereho se cte
def zacMnoziny(soubor):
    token=next(soubor)
    if token[0]!='carka':
        sys.stderr.write("Chybny format vstupniho souboru-chybi oddelovac carka")
        sys.exit(1)
    token=next(soubor)
    if token[0]!='zavorka_sloz':
        sys.stderr.write("Chybny format vstupniho souboru-chybi slozena zavorka jako zacatek dalsi mnoziny")
        sys.exit(40)

# funkce pro tisk vysledku v normovanem formatu
# na vstupu dostava vsechny mnoziny ve kterych jsou tisknutelna data
# funkce nic nevraci
def tiskni(stavy, abeceda, prechody, start, koncove_stavy):
    print("(\n{", end="")
    stavy.vypis(0)
    print("},\n{", end="")
    abeceda.vypis(1)
    print("},\n{\n", end="")
    prechody.vypis()
    print("},\n"+start+",\n{", end="")
    koncove_stavy.vypis(0)
    print("}\n)", end="")
