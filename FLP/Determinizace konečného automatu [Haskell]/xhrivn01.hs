-- Projekt č. 1 do předmětu FLP
-- Autor: Jan Hrivnák (xhrivn01@stud.fit.vutbr.cz)
-- Zadání: rka2dka - převod rozšířeného konečného automatu na deterministický
-- Datum: 04/2015
-- -------------------------

module Main(main) where

import System.IO
import System.Environment
import Data.Char
import Data.List
import System.Exit

import KAdata

-- ##################################       MAIN      ###############################--
main :: IO ()
main = do
	args <- getArgs
	let (simulate, inFile) = parseArgs args

	content <-
		if inFile=="stdin"
			then getContentStdin
			else getContentFile inFile
	ka <- getAutomat content

	if simulate 
		then printAutomat $ determinizujAutomat ka
		else printAutomat ka
	
	--hClose hInFile
	return ()
	where
		getContentStdin = hGetContents stdin
		getContentFile inFile = do
			hInFile <- openFile inFile ReadMode
			content <- hGetContents hInFile
			return content

--nacteni automatu
getAutomat :: String -> IO Automat
getAutomat content = do
	let lns = lines content --pole radku
	let ka = zpracujVstup lns
	return ka
-- ##################################    end MAIN     #################################--

determinizujAutomat :: Automat -> Automat
--determinizujAutomat ka = error $ show $ getEpsUzavery (stavy ka) (prechody ka) -- vypise EPS uzavery
--determinizujAutomat ka = odstranEpsilon ka $ getEpsUzavery (stavy ka) (prechody ka) -- vypise KA bez epsilon prechodu
determinizujAutomat ka = determinizace $ odstranEpsilon ka $ getEpsUzavery (stavy ka) (prechody ka) -- vypise DKA

------ #################################        ODSTRANENI EPSILON PRAVIDEL         ################################ ----

getEpsUzavery :: [KAstav] -> [Prechod] -> [StavDetail]--vraci EPS uzavery vsech stavu
getEpsUzavery [] p = []
getEpsUzavery (x:xs) prechody = (getEpsUzavery2 x prechody):getEpsUzavery xs prechody

getEpsUzavery2 :: KAstav -> [Prechod] -> StavDetail --vraci kompletni EPS uzaver jednoho stavu
getEpsUzavery2 st prechody= StavDetail st $ getEpsUzavery3 [st] prechody 

getEpsUzavery3 :: [KAstav] -> [Prechod] -> [KAstav]
getEpsUzavery3 stavy prechody = if (getUzaver stavy prechody "" []) == getUzaverUzaveru stavy prechody []
				then getUzaver stavy prechody "" []
				else getEpsUzavery3 (getUzaverUzaveru stavy prechody  []) prechody
	where
		-- zjisti EPS uzaver z pole stavu a pote jeste zjisti eps uzaver vysledku
		getUzaverUzaveru stavy p r = getUzaver (getUzaver stavy p "" r) p "" r

--vraci pole uzaveru pro mnozinu stavu a zadany znak
getUzaver :: [KAstav] -> [Prechod] -> KAznak -> [KAstav] -> [KAstav]
getUzaver [] _ znak ret = delete "" $ sort $ ret
getUzaver (x:xs) prechody znak ret = getUzaver xs prechody znak $ nub $ (get1Uzaver x prechody znak)++ret

--vraci stavy do kterych se dostanu jednim prechodem ze zadaneho stavu zadanym symbolem
get1Uzaver :: KAstav -> [Prechod] -> KAznak -> [KAstav] 
get1Uzaver st prechody zn= (map head . group . sort) $ getPrechody prechody
	where
		getPrechody prechody
			| zn=="" = st:(map isZnak prechody) --epsilon uzaver obsahuje i sam sebe
			| otherwise = map isZnak prechody
		isZnak prechod 
			| (stavFrom prechod)==st = 
				if (znak prechod)==zn
					then (stavTo prechod)
					else [] 
			| otherwise = [] 

-- dostane KA a dvojice (stav+jeho eps uzaver) na zaklade ktereho vraci KA bez epsilon prechodu
odstranEpsilon :: Automat -> [StavDetail] -> Automat
odstranEpsilon ka epsUzavery =  Automat (stavy ka) abc newPrechody (start ka) newEnd
	where
		abc=delete "" (abeceda ka)--odstranime "znak" epsilon z abecedy
		newPrechody = getNewPrechodyBezEps epsUzavery (delete (Prechod "Qfail" "Qfail" "Qfail") (map deleteEps (prechody ka))) []--odstranime prechody s Epsilonem a vygenerujeme "nove" pomoci uzaveru
		newEnd = nub $ (end ka)++(updateKonStavy epsUzavery (end ka) [])
		deleteEps prechod 
			| (znak prechod) /= "" = prechod
			| otherwise = Prechod "Qfail" "Qfail" "Qfail" --prepiseme si prechod na neco co pak jednoduse muzeme odstranit

		getNewPrechodyBezEps [] _ ret = ret
		getNewPrechodyBezEps (x:xs) prechody ret = getNewPrechodyBezEps xs prechody $  (mojePrechody x prechody)++ret 
		
--dostane (Stav+jeho epsilon uzaver) a vsechny prechody. Vraci nove prechody podle toho uzaveru
mojePrechody :: StavDetail -> [Prechod] -> [Prechod] 
mojePrechody uzaver prechody = getMojePrechody (stav uzaver) (zdroj uzaver) prechody [] 
	where
		--vraci vsechny prechody pro jeden dany stav
		-- stav -> jeho eps uzaver -> pole s vysledkem
		getMojePrechody origS [] _ ret = ret
		getMojePrechody origS (x:xs) prechody ret = getMojePrechody origS xs prechody $ nub $ (selMyPrechody origS x prechody [])++ret 

		selMyPrechody _ s [] r = r
		selMyPrechody origS s (p:ps) r 
			| (stavFrom p) == s = 	selMyPrechody origS s ps $ (Prechod origS (znak p) (stavTo p)):r 
			| otherwise =			selMyPrechody origS s ps r

--dostane stavy s jejich eps uzavery + puvodni koncove stavy -> vraci nove koncove stavy
updateKonStavy :: [StavDetail] -> [KAstav] -> [KAstav]-> [KAstav]
updateKonStavy [] _ ret = ret
updateKonStavy (x:xs) oldEnd ret = updateKonStavy xs oldEnd $ delete "" $ nub $ sort $ (jeStavMeziKoncovymi x oldEnd):ret
	where
		--overi jestli v eps uzaveru kazdeho stavu neni stav, ktery by byl v puvodni mnozine koncovych stavu
		jeStavMeziKoncovymi uzaver oldEnd
			| True `elem` (map (`elem` oldEnd) (zdroj uzaver) ) = (stav uzaver) -- map vraci pole [Bool] a hleda v nem alespon jedno True
			| otherwise = []

------ #######################################        DETERMINIZACE         ######################################## ----

determinizace :: Automat -> Automat
determinizace ka =  Automat getStavy (abeceda ka) getPrechody "0" getNewKoncoveStavy
	where
		--newStavy = error $ show $ pojmenujStavy (start ka) $ getNewStavy (abeceda ka) (prechody ka) [[(start ka)]]
		newStavy = pojmenujStavy (start ka) $ getNewStavy (abeceda ka) (prechody ka) [[(start ka)]]
		getStavy = sort $ delete [] $ map (stav) newStavy
		getPrechody = getNewPrechody newStavy newStavy (prechody ka) (abeceda ka) []
		getNewKoncoveStavy = updateKonStavy newStavy (end ka) []

getNewStavy :: [KAznak] -> [Prechod] -> [[KAstav]] -> [[KAstav]]
getNewStavy abc prechody soucStavy= if (getNewStavy2 soucStavy) == getNewStavy2 (getNewStavy2 soucStavy)
		then getNewStavy2 soucStavy
		else getNewStavy abc prechody (getNewStavy2 (getNewStavy2 soucStavy))
	where
		getNewStavy2 stavy = delete [] $ getNewStavy3 stavy abc prechody stavy

getNewStavy3:: [[KAstav]] -> [KAznak] -> [Prechod] -> [[KAstav]] -> [[KAstav]]
getNewStavy3 [] abc oldPrechody newStavy = newStavy
getNewStavy3 (x:xs) abc oldPrechody newStavy = getNewStavy3 xs abc oldPrechody $ rekurzeNewStavy x abc oldPrechody newStavy
	where
		rekurzeNewStavy stav abc prechody newStavy=  nub $ sort $ newStavy ++ (getNewDostupneStavy stav abc prechody [])

getNewDostupneStavy:: [KAstav] -> [KAznak] -> [Prechod] -> [[KAstav]] -> [[KAstav]]
getNewDostupneStavy stav [] prechody ret = sort $ nub $ ret
getNewDostupneStavy stav (z:zs) prechody ret = getNewDostupneStavy stav zs prechody [(getUzaver stav prechody z [])]++ret

--dostane pole poli kde kazde pole obsahuje seznam stavu, ze kterych se sklada jeden novy. Vraci tyto pole ale kazde jiz ma navic i svuj novy nazev podle zadani (cele kladne cislo)
pojmenujStavy:: KAstav -> [[KAstav]] -> [StavDetail]
pojmenujStavy start stavy = pojmenujStavy2 [(StavDetail "0" [start])] $ delete [start] stavy -- pocatecni stav je vzdy shodny s puvodnim pocatecnim a nove bude oznacen jako "0"
	where
		pojmenujStavy2 ret [] = ret
		pojmenujStavy2 ret (x:xs) = pojmenujStavy2 (( StavDetail (show((length ret))) x):ret) xs --stavy jsou cislovany od 0, takze staci pojmenovavat "length ret"

--dostane seznam novych stavu a starych prechodu -> vraci odpovidajici seznam novych prechodu mezi novymi stavy 
getNewPrechody:: [StavDetail] -> [StavDetail] -> [Prechod] -> [KAznak] -> [Prechod] -> [Prechod]
getNewPrechody _ [] prechody abc ret = ret
getNewPrechody allNewStavy (x:xs) prechody abc ret = getNewPrechody allNewStavy xs prechody abc $ ret++getNewPrechody2 allNewStavy (stav x) (zdroj x) abc prechody []
	where
		getNewPrechody2:: [StavDetail] -> KAstav -> [KAstav] -> [KAznak] -> [Prechod] -> [Prechod] -> [Prechod]
		getNewPrechody2 _ _ _ [] _ ret = ret
		getNewPrechody2 allNewStavy origS origZdroj (a:as) prechody ret = 
			getNewPrechody2 allNewStavy origS origZdroj as prechody 
				$ nub $ (getNewPrechody3 allNewStavy origS a (getNewPrechodyPro1Znak origS origZdroj a prechody []))++ret
		getNewPrechody3 allNewStavy origS znak dostupneStavy
			| null dostupneStavy = 	[]
			| otherwise =			[Prechod origS znak (findNewStav allNewStavy dostupneStavy)]

		--hleda mezi vsemi novymi stavy tan, jenz se sklada ze stejnych stavu jako jsou nase spocitane. Nemelo by se stat ze jej nenajde nebot v obbou pripadech jsou mnoziny stavu serazeny
		findNewStav [] hledanaMnaStavu = error "Cil prechodu pri determinizaci nenalezen mezi novymi prechody" --sem by se to nemelo nikdy dostat
		findNewStav (s:xs) hledanaMnaStavu
			| hledanaMnaStavu == (zdroj s) = (stav s)
			| otherwise = findNewStav xs hledanaMnaStavu

		--vraci pole puvodnich stavu dostupnych z danych stavu (nove jednoho) danym symbolem
		getNewPrechodyPro1Znak _ _ _ [] ret = sort $ nub $ delete [] ret
		getNewPrechodyPro1Znak origS origZdroj hledanyZnak (p:ps) ret 
			| (znak p) == hledanyZnak =	getNewPrechodyPro1Znak origS origZdroj hledanyZnak ps $ ret++( getNewPrechodyPro1Znak_2 origS origZdroj p ) 
			| otherwise = 				getNewPrechodyPro1Znak origS origZdroj hledanyZnak ps ret
		getNewPrechodyPro1Znak_2 origS origZdroj p
			| (stavFrom p) `elem` origZdroj = 	[(stavTo p)]
			| otherwise = 						[]

------ #######################################                   ######################################## ----

printAutomat :: Automat -> IO ()
printAutomat ka = putStrLn $ show ka

------ #######################################                   ######################################## ----

-- nactene radky "ulozi" jako automat
-- na vstupu ma pole radku
zpracujVstup :: [String] -> Automat
zpracujVstup (stavy:start:end:prechody) 
		| null prechody = error "Zadne prechody"
		| otherwise = Automat getStavy (getAbecedu prechody) (map getPrechody prechody) start getEndStavy
	where
		getStavy = splitStr stavy
		getEndStavy = splitStr end
		getPrechody p = getPrechod $ splitStr p

		getPrechod [q1,zn,q2] = Prechod q1 zn q2
		getPrechod _ = error "bad transition syntax"

		getAbecedu p = nub $ map getZnakZPrechodu p
		getZnakZPrechodu p = getZnakZPrechodu2 $ splitStr p

		getZnakZPrechodu2 [q1,zn,q2] = zn 
		getZnakZPrechodu2 _ = error "bad transition syntax"

zpracujVstup _ = error "spatna syntaxe vstupu"

--rozdeli string podle carky a vraci pole stringu
splitStr :: String-> [String]
splitStr text 
	| text == "" = []
	| otherwise = splitStr' text []
		where
			splitStr' [] r = [r]
			splitStr' (x:xs) t  
				| x==',' = 		t:splitStr' xs []
				| otherwise	= 	splitStr' xs (t++[x])

------ #######################################       1           ######################################## ----
--zpracovani vstupnich parametru
parseArgs :: [String] -> (Bool,String) --pole parametru prevede do dvojici
parseArgs [x]
	| x=="-i" = 	(False, "stdin")
	| x=="-t" = 	(True, "stdin")
	| otherwise = 	error "pouzitelne parametry -i a -t"

parseArgs [x,y]
	| x=="-i" = 	(False, y)
	| x=="-t" = 	(True, y)
	| otherwise = 	error "pouzitelne parametry -i a -t"
parseArgs _ = error "Spravne parametry jsou {-i|-t} nazev_souboru" 