import xmlrpc.client
import sys, re, base64, gzip, os , io
# Autor Jan Hrivnak
# ISJ projekt
# Soubor: Aplikace.py

# spoustet s : export LC_ALL=cs_CZ.UTF-8;
class Aplikace:
    def __init__(self):
        self.IMDBid=0 #IMDB id filmu
        self.subID=0 #ID zadanych titulku
        self.nazevFilmu=""
        self.auto=0
        self.url="" # zadana url adresa ceskeho titulku
        self.ost= xmlrpc.client.ServerProxy("http://api.opensubtitles.org/xml-rpc")
        self.login = self.ost.LogIn('', '', 'cs', 'xhrivn01-isj')
        self.token = self.login['token']
        self.vybraneCZ=[]
        self.vsechnyEN=[]
        self.czTitulky=[] #vyhledane ceske titulky                                     .
        self.enTitulky=[] # vyhledane anglicke titulky
        self.vysledek=[] #  pole pro ulozeni jednotlivych zarovnanych vysledku
        self.uspechy=[]
        
##############################################################################################################################
#                                            NACTI TITULKY
##############################################################################################################################
    def nactiTitulky(self, lang):
        #print("dotaz na server na ziskani seznamu titulku ("+lang+")")
        dotazNaOst = self.ost.SearchSubtitles(self.token, [{'sublanguageid':lang,'imdbid':self.IMDBid}])
        if dotazNaOst['status'] != "200 OK":
            sys.stderr.write("Server vratil chybu")
            sys.exit(1)
        elif dotazNaOst['data']==False:
            sys.stderr.write("Zadne titulky nenalezeny")
            sys.exit(1)
        if lang=="cze":
            print("Zakladni udaje o filmu:")
            print("\tFilm:", dotazNaOst['data'][0]['MovieName'])
            print("\tRok:", dotazNaOst['data'][0]['MovieYear'])
            self.czTitulky=dotazNaOst['data']
        else:
            self.enTitulky=dotazNaOst['data']
        print("uspesne nacteno", len(dotazNaOst['data']), "titulku ("+lang+")")
        return len(dotazNaOst['data'])

##############################################################################################################################
#                                                    VYBER CZ TITULEK
##############################################################################################################################
    def vyberCZtitulek(self):
        nacteneTitulky = []
        i=0
        print("vyber ceskych titulku:")
        print("----------------------------")
        for titulek in self.czTitulky:
            print(str(i)+'\t'+titulek['SubFileName'])
            nacteneTitulky.append([{'SubtitleName':titulek['SubFileName'],'imdbID':self.IMDBid,'SubtitleID':titulek['IDSubtitleFile']}])
            i+=1
        print("----------------------------")
        ok=0
        if self.auto==0:
            while(ok==0):
                try:
                    vyber=input('Zadejte cislo vybraneho titulku: ')
                    ok=1
                except:
                    sys.stderr.write("Nespravny vyber")
                    sys.exit(1)
                if int(vyber) >= i:
                    sys.stderr.write("Nespravny vyber")
                    sys.exit(1)
        else:
            vyber=0
            print("podle parametru --auto byly vybrany prvni titulky")
        self.vybraneCZ=nacteneTitulky[int(vyber)]
        print("uspesne vybrano")
        
##############################################################################################################################
#                                            VYBER EN TITULEK
##############################################################################################################################
    def vyberENtitulek(self):
        nacteneTitulky = []
        i=0
        for titulek in self.enTitulky:
            #print(str(i)+'\t'+titulek['SubFileName'])
            nacteneTitulky.append([{'SubtitleName':titulek['SubFileName'],'imdbID':self.IMDBid,'SubtitleID':titulek['IDSubtitleFile']}])
            self.vsechnyEN+=nacteneTitulky[i]
            i+=1
            
##############################################################################################################################
#                                                STAHNOUT
##############################################################################################################################
    def stahnout(self, idTitulku, nazev, lang):
        slozka="ISJ-xhrivn01-"+lang
        #print("Nachystani slozky pro ulozeni titulku:", slozka)
        if not os.path.exists(slozka):
            try:
                os.makedirs(slozka)
                print("vytvorime slozku "+slozka)
            except:
                sys.stderr.write("Chyba pri vytvareni adresare")
                sys.exit(1)
        stahnuto=self.ost.DownloadSubtitles(self.token, [idTitulku])
        if stahnuto['status']!="200 OK":
            sys.stderr.write("Nepodarilo se stahnout titulky")
            sys.exit(1)
        print("Stahuji titulky", nazev,"v",lang)
        try:
            dekodovane=base64.b64decode(bytes(stahnuto['data'][0]['data'], "cp1250"))
        except:
            sys.stderr.write("Nepodarilo se dekodovat titulky")
            sys.exit(1)

        text=str(gzip.decompress(dekodovane), "cp1250")
        try:
            file=open(slozka+'/'+nazev,'w', encoding="cp1250") #otevreme soubor pro zapis
        except:
            sys.stderr.write("Nepodarilo se otevrit soubor pro zapis")
            sys.exit(1)
        file.write(text)
        file.close()
        #print("titulky uspesne stazeny a ulozeny")
        
##############################################################################################################################
#                                            POROVNEJ
##############################################################################################################################
    def porovnej(self):
        czech=self.nacti(self.vybraneCZ[0]['SubtitleName'], "cze")
        posledniCZtit=czech[-1][0]['konec']

        for titulek in self.vsechnyEN:
            eng=self.nacti(titulek['SubtitleName'], "eng")
            #bereme v uvahu pouze ceske a anglicke titulky ktere maji cas posledniho titulku (=delky filmu) lisici se maximalne o 15 minut  (mel jsem zde 5 minuty, ale nektere titulky jsou delsi ptze maji i neco na konci pod "skutecnymi" titulkami ve filmu")
            posledniENGtit=eng[-1][0]['konec']

            if (posledniENGtit > posledniCZtit-900) and (posledniENGtit < posledniCZtit+900): # cislo je povolena tolerance ve vterinach
                print("Porovnavame titulky:",titulek['SubtitleName'])
                promluvy=self.seradPromluvy(czech, eng)
                if len(promluvy)!=0 and len(promluvy)>len(self.vysledek):  # je toto reseni lepsi nez predchozi?
                    self.vysledek=promluvy                                 # uloz ho

##############################################################################################################################
#                                            NACTI
##############################################################################################################################
    def nacti(self, nazev, lang):
        slozka="ISJ-xhrivn01-"+lang
        try:
            file=open(slozka+'/'+nazev,'r', encoding="cp1250")
        except:
            sys.stderr.write("Nepodarilo se otevrit soubor pro cteni")
            sys.exit(1)
        text=file.read()
        regSrt=re.compile("^.*\.srt$", re.DOTALL)
        regSub=re.compile("^.*\.sub$", re.DOTALL)
        if regSrt.match(nazev.lower()):
            #print("nacteme", nazev,"jako .srt titulky ("+lang+")")
            slovnik=self.srtTOslovnik(text)
        elif regSub.match(nazev.lower()):
            #print("nacteme", nazev,"jako .sub titulky ("+lang+")")
            slovnik=self.subTOslovnik(text)
        else:
            sys.stderr.write("Chybny format titulku. Podporovany jsou pouze formaty .srt a .sub")
            sys.exit(1)
        if len(slovnik)!=0:
            return slovnik
        else:
            sys.stderr.write("Nepodarilo se nacist a zpracovat soubor s titulky")
            sys.exit(1)
            
##############################################################################################################################
#                                                    STR TO SLOVNIK
##############################################################################################################################
    def srtTOslovnik(self, text):
        dotaz=re.compile("\d+\n\d{2}\:\d{2}\:\d{2},\d{3} --> \d{2}\:\d{2}\:\d{2},\d{3}\n.*?\n", re.DOTALL)
        titulky=dotaz.findall(text)
        vysl=[]
        for radek in titulky:
            match = re.search('(\d+):(\d+):(\d+),(\d{3}) --> (\d+):(\d+):(\d+),(\d{3})\n(.*?)\n',radek)
            if match != None:
                casZacatku = float(match.group(1))*3600+float(match.group(2))*60+float(match.group(3))+float(match.group(4))/1000
                casKonce = float(match.group(5))*3600+float(match.group(6))*60+float(match.group(7))+float(match.group(8))/1000
                text=match.group(9)
                text= re.sub('\n',' ',text)
                text= re.sub('[Mm]r\.','Mr',text)
                text= re.sub('[Mm]rs\.','Mrs',text)
                text= re.sub('[Mm]s\.','Ms',text)
                if len(text) > 0:
                    vysl.append([{'zacatek':casZacatku, 'konec':casKonce, 'text':text}])
        return vysl

##############################################################################################################################
#                                                    SUB TO SLOVNIK
##############################################################################################################################
    def subTOslovnik(self, text):
        dotaz=re.compile("{\d+}{\d+}.*?\n", re.DOTALL)
        titulky=dotaz.findall(text)
        vysl=[]
        for radek in titulky:
            try:
                start, end, text = re.findall("\{(\d+)\}\{(\d+)\}(.*)$", radek)[0]
            except:
                continue
            if len(text) > 0:
                vysl.append([{'zacatek':start/24, 'konec':end/24, 'text':text}])
        return vysl
        
##############################################################################################################################
#                                                    URCI POSUN
##############################################################################################################################
# vypocita posun anglicky titulku vuci ceskym.
#Vraci hodnotu ve vterinach
    def urciPosun(self, czech, eng):
        # Na zaklade prvniho vyskytu ? a ! spocita posun anglickych titulku vuci ceskym
        otaznikVtextu=re.compile(".*\?.*", re.DOTALL)
        for radek in czech:
            if otaznikVtextu.match(radek[0]['text']):  # ulozime si cas prvniho vyskytu otazniku v cz titulkach
                vCeskych=radek[0]['zacatek']
                break
        for radek in eng:
            if otaznikVtextu.match(radek[0]['text']):  # ulozime si cas prvniho vyskytu otazniku v en titulkach
                vAnglickych=radek[0]['zacatek']
                break
        posunOtaz=vCeskych-vAnglickych

        vykricnikVtextu=re.compile(".*\!.*", re.DOTALL)
        for radek in czech:
            if vykricnikVtextu.match(radek[0]['text']):
                vCeskych=radek[0]['zacatek']
                break
        for radek in eng:
            if vykricnikVtextu.match(radek[0]['text']):
                vAnglickych=radek[0]['zacatek']
                break
        posunVykric=vCeskych-vAnglickych
        return (posunOtaz+posunVykric)/2

##############################################################################################################################
#                                                    SERAD PROMLUVY
##############################################################################################################################
    def seradPromluvy(self, czech, eng):
        promluvy=[]
        posun=self.urciPosun(czech,eng)

        for cz in czech:
            ok=0 #poznamename si jestli aspon jedna promluva sedela k tomuto titulku. Pokud ne tak ji vypise samotnou

            #spocitame casy "okna" ve kterem budeme spojovat titulky
            start=cz[0]['zacatek']-posun
            konec=cz[0]['konec']-posun
            minZacatek=start-0.5 # tolerujeme rozmezi okna vterinu
            maxZacatek=start+0.5
            minKonec=konec-0.5
            maxKonec=konec+0.5

            for radek in eng:
                if radek[0]['zacatek']>minZacatek and radek[0]['zacatek']<maxZacatek and radek[0]['konec']>minKonec and radek[0]['konec']<maxKonec:
                    ok=1
                    cesky=cz[0]['text'].split(".")
                    anglicky=radek[0]['text'].split(".")
                    for veta in cesky:
                        if veta=="":
                            continue
                        vysl=self.vyberPromluvy(veta, anglicky)
                        if vysl==0:
                            promluvy.append(veta+"\t")
                        elif vysl==-1:
                            break
                        else:
                            promluvy.append(vysl)
            if ok==0: # zadna anglicka promluva nam k teto nesedi, tak ji vypiseme samostatne
                promluvy.append(cz[0]['text']+"\t")
        return promluvy
        
##############################################################################################################################
#                                            VYBER PROMLUVY
##############################################################################################################################
    def vyberPromluvy(self, veta, anglicky):
        ok=0
        slova=veta.split(" ")
        minSlov=len(slova)-2   #povolime o dve slova kratsi nebo delsi promluvu
        maxSlov=len(slova)+2
        for angVeta in anglicky:
            if angVeta=="":
                continue
            angSlova=angVeta.split(" ")
            if len(angSlova)>=minSlov and len(angSlova)<=maxSlov and slova!="" and angSlova!="":
                pridat= veta + " \t " + angVeta
                ok=1
                return pridat
        if ok==0: #anglicky ekvivalent nenalezen
            return 0
        else:
            return -1 #jiz jsme "preklad" nalezly tak neprochazime dalsi vety

##############################################################################################################################
#                                            VYPIS NEJLEPSI
##############################################################################################################################
    def vypisNejlepsi(self, soubor):
        file=open(soubor, "w", encoding="cp1250")
        if self.vysledek!=[]:
            for radek in self.vysledek:
                file.write(radek)
                file.write("\n")
            file.close()
        else:
            print("Nenalezeny zadne shody")
            
 ##############################################################################################################################
 #                                           ZJISTI Z URL
 ##############################################################################################################################
 # Nakonec tato funkce nejde. OST neustale vracelo chybu a nebylo mozne s snim takto spojit.
 # Je potreba skript spoustet se zadanim IMDB id(filmu)
    def zjistiZurl(self):
        try:
            id, nazev=re.findall("http://.*?/(\d+)/([a-z-]*)-[a-z]+$", self.url)[0]
            self.nazevFilmu = re.sub('-',' ',nazev)
            self.subID=id
            print(self.subID,self.nazevFilmu)
        except AttributeError:
            sys.stderr.write("Chybne zadany aprametr URL")
            sys.exit(1)
        back=self.ost.SearchMoviesOnIMDB(self.token, [self.nazevFilmu]);
        print(back['data'])
        self.IMDBid=re.sub('^[0]+','',back ['data'][0]['id'])
        back=self.ost.SearchSubtitles(self.token, [{'sublanguageid':lang,'imdbid':self.IMDBid}])
        if dotazNaOst['status'] != "200 OK":
            sys.stderr.write("Server vratil chybu")
            sys.exit(1)
        elif dotazNaOst['data']==False:
            sys.stderr.write("Zadne titulky nenalezeny")
            sys.exit(1)
        print("Zakladni udaje o filmu:")
        print("\tFilm:", dotazNaOst['data'][0]['MovieName'])
        print("\tRok:", dotazNaOst['data'][0]['MovieYear'])

        for titulek in dotazNaOst['data']:
            if titulek['IDSubtitle']==self.subID:
                nazev=titulek['SubFileName']
                break
        if nazev=="":
            sys.stderr.write("Chyba pri hledani titulku")
            sys.exit(1)
        self.stahnout(self.subID, nazev,'cze')