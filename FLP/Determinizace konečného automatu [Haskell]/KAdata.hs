-- Projekt č. 1 do předmětu FLP
-- Autor: Jan Hrivnák (xhrivn01@stud.fit.vutbr.cz)
-- Zadání: rka2dka - převod rozšířeného konečného automatu na deterministický
-- Datum: 04/2015
-- -------------------------

module KAdata where

type KAstav = String
type KAznak = String

myPrint::[String] -> String
myPrint (t:text) 
	| null text = t
	| otherwise	= t ++ "," ++ myPrint text
myPrint _ = ""

data Prechod = Prechod
	{ stavFrom :: 	KAstav
	, znak :: 		KAznak
	, stavTo :: 	KAstav
	} deriving (Eq)
instance Show Prechod where
	show (Prechod stav1 znak stav2) = "\n" ++ myPrint [stav1, znak,stav2]

-- obsahuje stav a jeho detail. Detail je vyuzit bud pro ulozeni EPS uzaveru daneho stavu 
-- nebo pri determinizaci jako nazev stavu a seznam puvodnich stavu ze kterych je tento novy slozen
data StavDetail = StavDetail
	{ stav :: 	KAstav
	, zdroj :: 	[KAstav]
	} deriving (Eq)
instance Show StavDetail where
	show (StavDetail stav zdroj) = stav ++ " - " ++ show zdroj

show2p [] = ""
show2p (x:[]) = show x
show2p (x:xs) = show x ++ show2p xs
data Automat = Automat
	{ stavy :: 		[KAstav]
	, abeceda :: 	[KAznak]
	, prechody :: 	[Prechod]
	, start :: 		KAstav
	, end :: 		[KAstav]
	} deriving (Eq)
instance Show Automat where
	show (Automat s a p st e) =  myPrint s ++ "\n" ++  st ++ "\n" ++ myPrint e ++ show2p p --abecedu nevypisujeme 
--	show (Automat s a p st e) =  "---stavy---\n" ++ myPrint s ++ "\n--abeceda--\n" ++ myPrint a ++ "\n---startovaci stav---\n" ++  st ++ "\n----koncove stavy---\n" ++ myPrint e ++"\n--- prechody---"++ show2p p
