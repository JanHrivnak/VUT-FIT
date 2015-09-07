/*
* 2. Projekt do předmětu FLP
* Zadání: Turingův stroj
* Autor: Jan Hrivnák (xhrivn01)
* Date: 05/2015
*/

read_line(L,C) :- % Reads line from stdin, terminates on LF or EOF.
	get_char(C),
	(isEOFEOL(C), L = [], !;
		read_line(LL,_),% atom_codes(C,[Cd]),
		[C|LL] = L).


isEOFEOL(C) :- % Tests if character is EOF or LF.
	C == end_of_file;
	(char_code(C,Code), Code==10).

read_lines(Ls) :-
	read_line(L,C),
	( C == end_of_file, Ls = [] ;
	  read_lines(LLs), Ls = [L|LLs]
	).

% ===============================================================
% Pomocné funkce %
% ===============================================================

%odstraní poslední prvek ze seznamu
delete([_], []).
delete([X|Xs], [X|Last]) :- delete(Xs, Last).

vypis_pasku([]) :- writeln.
vypis_pasku([H|T]) :- write(H), vypis_pasku(T).

vypis_pasky([]) :- true.
vypis_pasky([H|T]) :- vypis_pasku(H), vypis_pasky(T).

writeln(T) :- write(T), write('\n').
writeln :- write('\n').

retezec([],[]).
retezec([H|T],[C|CT]) :- atom_codes(C,[H]), retezec(T,CT).

%%% každý řádek má bud formát Stav<mezera>Znak<mezera>Stav<mezera>Znak nebo jej bereme jako text (vstup pásky)
split_line([S1|[_|[Z1|[_|[S2|[_|[Z2]]]]]]], [S1,Z1,S2,Z2]).
split_line(S, R) :-	string_to_atom(S, R).

% vstupem je seznam radku (kazdy radek je seznam znaku)
split_lines([],[]).
split_lines([L|Ls],[H|T]) :- split_lines(Ls,T), split_line(L,H).

% ===============================================================
% Turingův stroj %
% ===============================================================

%%% Zjistí v jakém stavu se aktuálně nacházíme
getStav([H|ZbytekPasky],S,Z) :- 
	(	char_type(H, upper), % Stavy jsou označovány velkými písmeny
		S=H,
		(	ZbytekPasky==[], % jsme na konci, takže znak za Stavem je mezera (nekonečná)
			Z=' '
		;
			ZbytekPasky=[Z|_]
		)
	;
		getStav(ZbytekPasky,S,Z)
	).

%%% Posun na pásce doprava
posunPaskuR([S1], [S1|[_|[S2|[_]]]], R) :- %stav je až na konci pasky, musíme přidat "nekonečnou" mezeru
	append([' '],[S2],R). % toto by se muselo odstranit/upravit pokud bychom chtěli simulovat TS s omezenou délkou pásky
posunPaskuR([H|T], [S1|[Z1|[S2|[Z2]]]], R) :-
	(	H==S1,
		[StaryZnakPodHlavou|TT] = T,
		append([StaryZnakPodHlavou],[S2],R2),
		append(R2,TT,R)
	;
		posunPaskuR(T,[S1,Z1,S2,Z2], NewR),
		R=[H|NewR]
	).

%%% Posun na pásce doleva
posunPaskuL([H|T], [S1|[Z1|[S2|[Z2]]]], R) :-
	(	H==S1, % hledany stav je hned prvním znakem, nemůžeme se posunout doleva
		writeln('ERROR: preteceni pasky doleva'), 
		halt
	;
		[T1|TT] = T,
		(	T1==S1, % nasledující stav je aktuální stav 
			append([S2],[H],R2), % prohodíme jej(resp. nový Stav) z aktuálním znakem v H
			append(R2,TT,R) % a zkopírujeme zbytek pásky
		;
			posunPaskuL(T,[S1,Z1,S2,Z2], NewR),
			R=[H|NewR]
		)
	).

%%% upraví pásku pomocí zadaného pravidla
upravPasku([S1],[S1|[_|[S2|[Z2]]]],R):- %jsme již na konci pasky. Novy stav a znak tedy zapiseme "za" aktuální pásku nebot máme neknečnou pasku doprava
	append([S2],[Z2],R).
upravPasku([H|T],[S1|[Z1|[S2|[Z2]]]],R):-
	(	H==S1,
		[_|TT] = T, % odstraníme aktualni znak pod hlavou
		append([S2],[Z2],R2), % dáme na pásku nový stav a nový znak pod hlavu
		append(R2,TT,R) % zkopírujeme zbytek pásky
	;
		upravPasku(T,[S1,Z1,S2,Z2], NewR),
		R=[H|NewR]
	).

%%% nalezne pravidlo pro aktuální stav a aktuální znak pod hlavou
getPravidlo([H|T_pravidla],ActStav,ZnakPodHlavou,R) :-
	[S1,Z1,_,_] = H,
	(	S1==ActStav, % máme pravidlo začínající ve stejném stavu
		Z1==ZnakPodHlavou, % a pokračující stejným symbolem
		R=H % je to pravidlo které hledáme
	;
		getPravidlo(T_pravidla,ActStav,ZnakPodHlavou,R)
	).


%%% Provede další krok v simulaci pásky. Modifikuje vstupní pásku podle zadaých pravidel
simulate(Paska, Pravidla, Cesta):-
	getStav(Paska, ActStav, ZnakPodHlavou),
	(	ActStav=='F', % cool, již jsme na konci a máme výslednou pásku
		true
	;
		getPravidlo(Pravidla,ActStav,ZnakPodHlavou,VybranePravidlo),
%		write('nalezene pravidlo: '), writeln(VybranePravidlo),
		[_,_,_,NewZnak] = VybranePravidlo,
		(	NewZnak=='L', % znak pro posun doleva
			posunPaskuL(Paska, VybranePravidlo, NovaPaska)
		;
			NewZnak=='R', % znak pro posun doprava
			posunPaskuR(Paska, VybranePravidlo, NovaPaska)
		; % nový znak je klasický znak
			upravPasku(Paska, VybranePravidlo,NovaPaska)
		),
%		write('Nova paska: '), vypis_pasku(NovaPaska),
		simulate(NovaPaska, Pravidla, NovaCesta), % pokračujeme v simulaci s novou páskou
		Cesta = [NovaPaska|NovaCesta] % nalezenou pásku si uložíme pro pozdější vypsání
	).


start :-
		prompt(_, ''),
		read_lines(LL),
		split_lines(LL,Vstup),
		last(Vstup,P1), % výběr vstupu pásky (poslední řádek)
		string_to_list(P1,P2), 
		retezec(P2,P3),
		append(['S'],P3,Paska), % přidáme počáteční stav S
		delete(Vstup, Pravidla), % odebrání posledního řádku ze vstupního textu (vstup pásky)
		vypis_pasku(Paska), % vypsání prvního řádku
%		writeln(Pravidla),
%		writeln('------- zacatek vypoctu ---------'),
		!,
		simulate(Paska,Pravidla,Vysledek),
		vypis_pasky(Vysledek).