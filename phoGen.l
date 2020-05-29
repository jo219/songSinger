%{

/*//////////////////////////////////////////////////////////
IND Phoneme Generator
Jonathan, Elins UGM 2014

Buglist:
*Keynote: Octave and Chromatic
*Indeks LIR and NOT belom dinamis

*Gmn cara variabel sama gaperlu disebut berkali2, pas printf
*///////////////////////////////////////////////////////////

#include "phoGen.h"
%}

%%

\<<		return COMMENT;

\<L		return LYR_IN; //
>L		return LYR_EX; //

\<N		return NOTE_IN; //ART;
>N		return NOTE_EX; //END

\|		return BAR;
\(		return DIP; //DIVIDER;
\(t		return TRIP; //TRIVIDER;
\)		return PART_EX; //MULTIPLY_BACK
\.		return EXTEND; //EXTENDER
\[		return LEGATO; //SEMIQ
\] 		return LEG_EX; //SEMIQ_EX
,		return OCT_DW; //OCTAVE_DOWN;
'		return OCT_UP; //OCTAVE_UP;
\\		return CHRO_DW; //CHROMATIC_DOWN;
\/		return CHRO_UP; //CHROMATIC_UP;
\|\}		return END_BAR;
[0-7]{1}	return NOTE;

[ \t]		return SPACE; //SPASI;
\n		return ENTER; 
\-		return HYPHEN; //SYLABLE_PART;

\#[Dd][Oo][ \t]*=[ \t]*[A-Za-z][is]?	return KEYNOTE;
\#[Mm][Mm][ \t]*=[ \t]*([0-9])+\.	return TEMPO;
\#[Bb][ \t]*=[ \t]*[0-9]\/[0-9]		return BEATBAR;

[AEIOUaeiou]				return VOCAL;	//	
([aA]i)|([Aa]u)|([Ee]\^)		return DIPH;	//DIFTONG	//eo //
([kK]h)|([nN]g)|([nNsS]y)		return MUL_CONS; //GABUNGKONS; 
[B-DF-HJ-NP-TW-Zb-df-hj-np-tw-z]	return CONS; //KONSONAN;
.					return UNKNOWN;

%%

int yywrap(void) {
        return 1;
}

