Skript: ISJ 2011/2012 Automatické stahování a srovnávání titulků
Autor: Jan Hrivnák
Kontakt: xhrivn01@stud.fit.vutbr.cz

Spuštění:
./isj.py --id=884328
parametrem předané IMDB ID filmu. Skript poté vyzve uživatele k vybrání z nabídnutých českých titulků. Anglické titulky se porovnají všechny vhodné a vybere se to nejlepší řešení.

./isj.py --id=884328 --auto
Stejné jako předchozí, ale skript nevyžaduje další zásah uživatele a sám automaticky vybere některé české titulky pro které provede zarovnání

./isj.py -h zobrazí nápovědu

Řešení:
řešení bylo zvoleno po konzultaci s některými studenty, kteří již s tímto měli zkušenosti.
Nakonec je implementováno přiřazování následovně:
- Z porovnávání se vyřadí titulky které mají výrazně jinou délku (např. verze rozdělené na více částí proti sobě určitě celé sedět nebudou)
- Podle prvního výskytu speciálních znaků (?, !) se vypočítá posunutí celých titulků (cz/en)  proti sobě. Tím se eliminuje například, když jedny titulky začínají např. o minutu dřív.
- Výpočet probíhá postupně pro každou českou promluvu. Vůči jeho časování se vypočítá +-okno. V anglické verzi se vyhledají titulky, časově spadající do tohoto časování
- Na základě počtu slov se provede přiřazení
- Pokud se žádná promluva nenajde, tak se vypíše pouze samotná česká.
- Výsledek se uloží pouze v případě, že došlo k přiřazení většího počtu promluv jak při předchozím eng titulku, jinak se zahodí
- Nejlepší řešení se nakonec zapíše do souboru

Vyhodnocení:
- Poskytnutý soubor na porovnání má bohužel opačné řazení (eng-cz), tudíž jsem porovnání nedělal
- ručním projitím výsledného přiřazení bych určil správnost na zhruba 80%. Výsledek je ale závislý na výběru správných vstupních souborů a jejich správném načasování
- Pro otestování na doporučené sadě je potřeba skript spustit jako "./isj.py --id=884328" a ve výběru zvolit titulky číslo 3 (Za předpokladu, že na serveru nepřibydou další, které by pořadí načítání pozměnili)

Pokračování:
- V případě dalšího rozšiřování funkčnosti skriptu bych se zaměřil na zprovoznění spuštění skriptu se zadáním URL adresy konkrétních titulků. Tuto funkčnost jsem již zkoušel, ale server opensubtitles.org je velmi nestabilní a otestování se pravidelně zdařilo až na několikátý pokus a nakonec tedy tato část nebyla plně funkční a v odevzdávané verzi je tato část zakomentována.
- Pro zlepšení řpřazování by jistě prospělo upravit heuristiku aby nepočítala pouze počet slov, ale také například koncové znaky (?! apod), případně rozpoznávání Velkých písmen, například u jmen.
