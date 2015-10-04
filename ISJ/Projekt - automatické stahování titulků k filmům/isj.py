#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Autor: Jan Hrivnak (xhrivn01(at)stud.fit.vutbr.cz)
# Projekt ISJ - 2011/2012
# Datum: 4-5/2012
# Skript ze zadaneho IMDB ID filmu stahne ceske a naglicke titulky
# nasledne zarovna vedle sebe odpovidajici vypovedi a ulozi je do jednoho souboru vystup.txt

import sys, gzip, re, os, getopt, shutil
import xmlrpc.client
from Aplikace import *

if __name__ == "__main__":
    program=Aplikace()
    print("Prihlaseni k OpenSubtitles.org ")

    if len(sys.argv)<2 or len(sys.argv)>3:
        sys.stderr.write ("Spatne vstupni parametry. Spustte program s -h")
        sys.exit(1)
    try:
        opts, args = getopt.getopt(sys.argv[1:], "ha", ["help", "auto", "id=", "url="])
    except (getopt.GetoptError):
        sys.stderr.write ("Spatne vstupni parametry. Spustte program s -h")
        sys.exit(1)

    for options, param_text in opts:
        if options in ("-h", "--help"):
            print("ISJ projekt - zarovnavani titulku")
            print("Autor: Jan Hrivnak-xhrivn01(at)stud.fit.vutbr.cz")
            print("Spusteni:")
            print("--id=XXX IMDB id filmu")
            #print("--url=\"http://...\" zadani URL adresy ceskych titulku")
            #print("tyto dva parametry neni povoleno kombinovat!")
            print("--auto nebo -a Automaticky rezim. Skript sam zvoli ceske titulky a neocekava od uzivatele dalsi vstup")
            print("\n --help - zobrazi tuto napovedu k programu")
            sys.exit() # vytistena napoveda, koncime

        elif options in ("-a", "--auto"):
            program.auto=1
            
        elif options == "--id":
            try:
                program.IMDBid=int(param_text)
            except:
                sys.stderr.write ("Spatne zadany parametr ID")
                sys.exit(1)
            if program.IMDBid==0:
                sys.stderr.write ("Spatne zadany parametr ID")
                sys.exit(1)

        elif options=="--url":
            sys.stderr.write ("Zadani URL adresy neni podporovano. OST mi stale vrace chybu pri jednom z dotazu")
            sys.exit(1)
            
        else:
            sys.stderr.write("Chybne zadane parametry. Pro napovedu spustte program s -h")
            sys.exit(1)

    pocet=program.nactiTitulky("cze")
    program.vyberCZtitulek()
    program.stahnout(program.vybraneCZ[0]['SubtitleID'], program.vybraneCZ[0]['SubtitleName'],'cze')
    print("----------------------------")
    pocet=program.nactiTitulky("eng")

    program.vyberENtitulek()
    print("stahneme vsechny eng titulky")
    i=0
    for i in range(pocet):
        #print("stahuji titulek " + program.vsechnyEN[i]['SubtitleName'])
        program.stahnout(program.vsechnyEN[i]['SubtitleID'], program.vsechnyEN[i]['SubtitleName'], 'eng')
    print("----------------------------")
    print("Porovnavame pouze titulky podobne delky")
    program.porovnej()
    program.vypisNejlepsi("vystup.txt")
    print("----------------------------")
    print("Nejlepsi vysledne porovnani ulozeno do souboru")
    shutil.rmtree('./ISJ-xhrivn01-cze')
    shutil.rmtree('./ISJ-xhrivn01-eng')
    print("Docasne adresare s titulkami odstraneny")
    print("#############  KONEC VYPISU  ###############")
