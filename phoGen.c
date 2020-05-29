/*//////////////////////////////////////////////////////////
IND Song Phoneme Generator
Jonathan, Elins UGM 2014

Buglist:
*BeatBar //solved pragmaticaly
*Ketukan nambah sama Konsonan2 //solved pragmaticaly
*Indeks LIR and NOT belom dinamis //kasih konstanta
*Konstanta buat ukuran silabel?? [8 aja]
*tambahan: fitur buat baca beatTime, konsekuensi ke durasi xV [OK]
*tambah error pecahan terlalu kecil (maksimal 3 kali partition aja, mainin langsung di body DIP sama TRIP)
*Exit bar belum

23/10/2017
*///////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "phoGen.h"

extern int yylex();
extern char *yytext;

/////Database of LIR and NOT

char *fonRef[] 	= { "V", "b", "tS", "d", "e" , \
		    "f", "g", "h" , "I", "dZ", \
		    "k", "l", "m" , "n", "Q" , \
		    "p", "k", "r" , "s", "t" , \
		    "U", "f", "w" , "z", "j" , "z", \
		    "@", "@", "aI","aU", "N" ,"nY", "h", "s"};

char *MCnD[] 	= { "E^", "e^", "ai", "au", "ng", "ny", "kh", "sy"};

int noteC[] 	= { 0, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247};  //C3 kromatik
int noteRef[18]; 

int transCode[] = { 0, 1, 3, 5, 6, 8, 10, 12, 0, 0, -1, 2, 4, 6, 7, 9, 11, 13}; //Krom to Std

/////////PROGRAM MAIN VAR

#define MAX_NOTE 300

char Lyr[MAX_NOTE][8][3];	//%c pake bintang, %s gausah
int  ftNote[MAX_NOTE][3];	//0 freq; 1 time; 2 Leg

int ntoken;

//////cuma dipake di Lyr
int  con_Lyr[2] = {0, 0}; //0 phoneme, 1 sylable
int  f_pVoc=0;
union uFo {char fo; char mcFo[2]; int iFo;};
union uFo fon;
union uFo fonPrev;

//////cuma dipake di Not
int  con_Note=0;
int  con_DT=0;
int  con_bar=0;
int  beatTime=0;
int  time, nTime=0;
int  noteTable[25];
char nNote=8;		//int frNote;
int  barBeat=0;
int  barTime=0;
int  DT[3] = {0, 0, 0};
int  octave=0;
int  leg=0, nLeg=0;

///////...........Proses Akhir
int con_FN;   		//Counter indeks akhir
int con_xV;
int con_legN;		//legato
int con_zeroN;	   	//Hindarin freq 0 repeat >5x
int con_zeroL;
int con_zeroNL;
int con_MC;
int xvTime = 0;
int phoNote;	   	//Indeks vokal terakhir, per silabel ganti
int f_notePrint=0; 	//Apakah note sudah dibuang (per silabel)
int f_endBarPresence=0;

int con_Err=0;
int conCon=0;

//////PROGRAM

int pwr(int x, int y) {
	for(int i=1; i<y; i++) x*=x;
	return x;
}

void renew_noteRef(int keynote, int board) {
	for(int i=0; i<13; i++) {
		noteTable[i] = noteC[i];
		noteTable[i+12] = noteC[i]*2;
	}

	for(int i=0; i<18; i++) {
		if	  (transCode[i]==-1) noteRef[i] = noteTable[transCode[7]+keynote-1]/2;
		  else if (transCode[i]!= 0) noteRef[i] = noteTable[transCode[i]+keynote-1];

		if	  (board>0) noteRef[i]*= pwr(2,  board); //naik oktaf, ga dipake
		  else if (board<0) noteRef[i]/= pwr(2, -board); //turun oktaf, ga dipake
	}
}

void storeToFT() {
	int frNote = noteRef[nNote-48];
	if(octave!=0) {;
		if(octave<0) frNote/=(-octave+1);
		  else	     frNote*=( octave+1);
	}
	ftNote[con_Note][0] = frNote;
	ftNote[con_Note][1] = nTime;
	ftNote[con_Note][2] = nLeg;
}

void errorList(int i) {
	con_Err++;

	FILE *e = fopen("errorLog", "a");
	if(i<10) fprintf(e, "\nError 0%d: ", i);
	  else	 fprintf(e, "\nError %d: ", i);
	switch(i) {
		case 1: fprintf(e, "Keynote not found\n Set note to Do=C (default)"); break; 
		case 2: fprintf(e, "Time sign not found\n Set time sign to %d/4 (default)", barBeat); break;
		case 3: fprintf(e, "Tempo not found\n Set tempo to %d bpm (default)", (1000*60)/beatTime); break;

		case 4: fprintf(e, "Total duration faulty at bar %d\n It's %d ms not %d ms!", con_bar, barTime, barBeat*beatTime); break;
		case 5: fprintf(e, "Partition sign in bar %d is not completed (unclosed)\n The beat is %d ms not %d ms!", con_bar, time, beatTime); break;
		case 6: fprintf(e, "Partition sign in bar %d is not completed (unopened)\n The beat duration is %d ms not %d ms!", con_bar, time, beatTime); break;
		case 7: fprintf(e, "Partition sign in bar %d limited to 2 layers only\n\nProgram exited without build the PHO\n\n", con_bar); break;
		case 8: fprintf(e, "Legato sign unclosed in bar %d", con_bar); break;
		case 9: fprintf(e, "Legato sign unopened in bar %d", con_bar); break;
		case 10: fprintf(e, "Chromatic/octave sign in bar %d is not worked for the rest sign", con_bar); break;
		case 11: fprintf(e, "Musical notation \"%s\" in bar %d is unlisted!", yytext, con_bar); break;
		case 12: fprintf(e, "Alphabet character \"%s\" for the lyric is unlisted!", yytext); break;
		case 13: fprintf(e, "Can't find \">N\" after \"<N\"\n\nProgram exited without build the PHO\n\n"); break;
		case 14: fprintf(e, "Can't find \">L\" after \"<L\"\n\nProgram exited without build the PHO\n\n"); break;
		case 15: fprintf(e, "Too much notes"); break;
		case 16: fprintf(e, "Too much lyrics"); break;
		case 17: fprintf(e, "Can't find the end of Bar \'|}\'"); break;
		case 18: fprintf(e, "String \"%s\" is out of syntax of song text", yytext); break;
		default: fprintf(e, "another unknown error");
	}
}

void error_check1() {
	if(noteRef[2]==0) {renew_noteRef(1, 0); errorList(1);}
	if(barBeat==0) 	{barBeat=4; errorList(2);}
	if(beatTime==0) {
		beatTime=1000; time=beatTime; 
		xvTime = beatTime/12; errorList(3);
	}	
}

void set_keynote() {
	char buff[3]; // 0=A-F 1=boardNum 2=..is

	for(int i=0, j; i<strlen(yytext); i++) {
		if((*(yytext+i)=='=' || *(yytext+i)==' ') && 	  \
		   ((*(yytext+i+1)>='A' && *(yytext+i+1)<='Z') || \
		    (*(yytext+i+1)>='a' && *(yytext+i+1)<='z')) ) { 
			buff[0]=*(yytext+i+1); buff[1]=4;//*(yytext+i+2);
			if(*(yytext+i+2)=='s' || *(yytext+i+2)=='i') {buff[2]='s';}
		}
	}

	if(buff[0] > 96) buff[0]-=32;	//biar jadi huruf gede
	buff[0]-=66;			//C=1
	if(buff[0] < 1 ) {buff[0]+=7; buff[1]=3;}	//A=6
	if(buff[2]=='s') buff[0]+=10;	//cis=11

	buff[0] = transCode[buff[0]];
	buff[1]-=4;

	renew_noteRef(buff[0], buff[1]);
}

void set_barBeat() {
	for(int i=0, j; i<strlen(yytext); i++) {
		if(*(yytext+i)=='/') {
			barBeat=*(yytext+i-1); barBeat-=48;
		}
	}
}

void set_tempo() {
	char buff[5];
	for(int i=0, j; i<strlen(yytext); i++) {
		if((*(yytext+i)=='='   || *(yytext+i)==' ') &&\
		   (*(yytext+i+1)>='0' && *(yytext+i+1)<='9')) { 
			j=i; buff[i-j] = *(yytext+i+1); 
			while(*(yytext+i+1)!='.') {
				i++; buff[i-j] = *(yytext+i+1);
			}
		}
	}
	beatTime = 1000*60/(atoi(buff)); //bpm to ms/beat
	xvTime = beatTime/12;
	time = beatTime;
}

void get_lyr() {
	if(ntoken==HYPHEN||ntoken==SPACE||ntoken==ENTER) {
		con_Lyr[1]++; con_Lyr[0]=0; ////1=Sylable 0=Phoneme
	} else { if(ntoken<VOCAL || ntoken>CONS) errorList(12); else {
		char j;
		if(ntoken==VOCAL||ntoken==CONS) { 	//Anti-Do-KONS
			fonPrev.fo = fon.fo;
			fon.fo = *yytext;
			if(fon.fo<97) fon.fo+=32; //To lowercase
		} else if(ntoken==MUL_CONS || ntoken==DIPH) {	
			fonPrev.fo = fon.fo;	//
			strcpy(fon.mcFo, yytext);
			for(char i=0; i<8; i++) {
				if (strcmp(MCnD[i], fon.mcFo)==0) j=i;
			}
			fon.fo = j+26+97;
		}

		if(((ntoken==VOCAL || ntoken==DIPH) && f_pVoc==1) || fonPrev.fo==fon.fo) { //Anti Do-KONS / Do-VOC
			f_pVoc = 0;
			strcpy(Lyr[con_Lyr[1]][con_Lyr[0]], "_");
			con_Lyr[0]++;
		}

		if(ntoken==MUL_CONS || ntoken==DIPH) {
			if(ntoken==MUL_CONS && con_MC==1) {
				strcpy(Lyr[con_Lyr[1]][con_Lyr[0]], "_");
				con_Lyr[0]++; con_MC=0;
			}
			strcpy(Lyr[con_Lyr[1]][con_Lyr[0]], fonRef[j+26]);
		} else strcpy(Lyr[con_Lyr[1]][con_Lyr[0]], fonRef[fon.fo-97]);	//Pho take!

		con_Lyr[0]++;

		if(ntoken==VOCAL || ntoken==DIPH) f_pVoc=1; else f_pVoc=0;
		if(ntoken==MUL_CONS) 		  con_MC=1; else con_MC=0;
		if(ntoken==NOTE_IN)	{errorList(14); exit(0);}
		//} else if(!ntoken)	{errorList(5); break;
		//}
	}}
	ntoken = yylex();
}

void get_note() {
	if (ntoken==NOTE) {
		if (nNote!=8) {
			storeToFT(); //n-n ke ftNote[con_Note][1:3]
			con_Note++;
		}

		nNote = *yytext;
		nTime = time;
		nLeg  = leg;
		barTime+=time;
		octave=0;

	} else {
		if(ntoken==BAR) {
			if((barTime<((beatTime*barBeat)-(beatTime/9)) || \
			    barTime>((beatTime*barBeat)+(beatTime/9))) && con_bar>0)
				errorList(4);
			if	  (time<=(beatTime/2)) errorList(5);
			  else if (time>=(beatTime*2)) errorList(6);
			barTime=0; con_bar++;
			time=beatTime;
		} else if(ntoken==DIP) {
			if(con_DT==2) errorList(7);
			  else {time/=2; con_DT++; DT[con_DT]=2;
			}
		} else if(ntoken==TRIP) {
			if(con_DT==2) errorList(7);
			  else {time/=3; con_DT++; DT[con_DT]=3;
			}
		} else if(ntoken==PART_EX) {
			if		(DT[con_DT]==2) {time*=2; con_DT--;}
			  else if	(DT[con_DT]==3) {time*=3; con_DT--;}
		} else if(ntoken==EXTEND) {nTime+=time; barTime+=time;
		} else if(ntoken==CHRO_DW) {
			if(nNote!='0') nNote+=9; else errorList(10);
		} else if(ntoken==CHRO_UP) {
			if(nNote!='0') nNote+=10; else errorList(10);
		} else if(ntoken==OCT_DW) {
			if(nNote!='0') octave--; else errorList(10);
		} else if(ntoken==OCT_UP) {
			if(nNote!='0') octave++; else errorList(10);
		} else if(ntoken==LEGATO) leg=1;
		  else if(ntoken==LEG_EX) {
			if(leg==1) {leg=0; nLeg=2;} else errorList(9);
		} else if(ntoken==END_BAR) f_endBarPresence=1;//{ftNote[con_Note+1][1] = 8;}
		  else if(ntoken==SPACE||ntoken==ENTER) ;
		  else if(ntoken==LYR_IN)	{errorList(13); exit(0);
		//} else if(!ntoken)		{errorList(5); break;
		} else errorList(11);
	}
	ntoken = yylex();
}

void printPHO() {
	FILE *f = fopen("file.pho", "w");
	for(int i=0; i<MAX_NOTE; i++) {
		con_xV=0;
		int legTNote[3] = {0, 0, 0}; //0=buff 1=current 2=total

		//Hindarin error error
		if(Lyr[i][0][0]=='\0' && ftNote[con_FN][0]==0) con_zeroNL++; else con_zeroNL=0;
		if(con_zeroNL>5) break;

		//Hidarin lyr 0
		if(Lyr[i][0][0]=='\0') con_zeroL++; else con_zeroL=0;
		if(con_zeroL>5) {errorList(15); break;} //overnote

		///Hindarin not 0
		if(ftNote[con_FN][0] == 0) {//&& con_zeroN<6) {
			if(con_zeroN<5) {
				fprintf(f, "_\t %d\n", ftNote[con_FN][1]);
				con_FN++; con_zeroN++;
			} else {errorList(16); break;} //overlyr [masih gabisa]
		}  else con_zeroN=0;

		//Kasih indeks fonem
		f_notePrint=0;
		for(int j=7; j>=0; j--) {
			char lyrD = *Lyr[i][j];
			if(Lyr[i][j][0] != '\0') { 
				if(strchr("VeIQU@a", lyrD) && f_notePrint==0) {
					phoNote=j; f_notePrint=1;
				} else if(lyrD=='_') {;
				} else con_xV++;
			}
		}

		//cetak PHO
		for(int j=0; j<8; j++) { if(Lyr[i][j][0] != '\0' && ftNote[con_FN][0]!=0) {
			if(j==phoNote) { //Vocal sound proccess
				conCon++;
				if(ftNote[con_FN][2]==0) {
					fprintf(f, "%s\t %d 0 %d 100 %d\n", Lyr[i][j], \
					 ftNote[con_FN][1]-(xvTime*con_xV), ftNote[con_FN][0], ftNote[con_FN][0]);
					con_FN++;
				} else /*if(ftNote[con_FN][2]==1)*/ {
					con_legN=con_FN; 
					legTNote[2]=ftNote[con_FN][1]; 
					con_FN++;
					while(ftNote[con_FN-1][2]!=2) {// && ftNote[con_FN][2]==1) {
						legTNote[2]+=ftNote[con_FN][1];  //dapetin tootal durasi
						con_FN++;
					}
					fprintf(f, "%s\t %d", Lyr[i][j], legTNote[2]-(xvTime*con_xV)); //print durasi
					for(int k=con_legN; k<con_FN; k++) { //print not
						fprintf(f, " %d %d", legTNote[1], ftNote[k][0]);
						legTNote[0]=ftNote[k][1]*100/legTNote[2];
						legTNote[1]+=legTNote[0];							
						fprintf(f, " %d %d", legTNote[1]-1, ftNote[k][0]);
					}
					fprintf(f, " 100 %d\n", ftNote[con_FN-1][0]);
				}
			} else {//other phoneme process
				//fprintf(f, "%s\t 65 0 %d 100 %d\n", Lyr[i][j], ftNote[con_FN][0], ftNote[con_FN][0]);
				if(strchr("_", *Lyr[i][j])) 
					fprintf(f, "%s\t 1\n", Lyr[i][j]);
				  else {
					fprintf(f, "%s\t %d\n", Lyr[i][j], xvTime);
					/*if(ftNote[con_FN-2][2]==1 && ftNote[con_FN-1][2]==0)
						fprintf(f, "%s\t %d\n", Lyr[i][j], legTNote[2]*2/10/con_xV);
					else
						fprintf(f, "%s\t %d\n", Lyr[i][j], ftNote[con_FN-1][1]*2/10/con_xV);*/
				}
			}
			//con_zeroN=0;
		} }
	}
	fprintf(f, "_\t 2\n");
}

int main(void) {

	FILE *a = fopen("errorLog", "w");
	fclose(a);
	FILE *e = fopen("errorLog", "a");

	ntoken = yylex();
	while (ntoken) {
		if (ntoken==COMMENT) { ntoken=yylex(); while(ntoken!=ENTER) ntoken=yylex();
		} else if (ntoken==KEYNOTE) set_keynote();
		  else if (ntoken==BEATBAR) set_barBeat();
		  else if (ntoken==TEMPO)   set_tempo();
		  else if (ntoken==LYR_IN) {
			ntoken = yylex(); 
			while(ntoken!=LYR_EX) get_lyr();
		} else if(ntoken==NOTE_IN) {
			error_check1();
			ntoken = yylex(); 
			while(ntoken!=NOTE_EX) get_note();
		} else if(ntoken==ENTER || ntoken==SPACE) ;
		  else errorList(18);
		ntoken = yylex();
	}

	storeToFT(); 
	con_Note++;

	if(leg==1) errorList(8);
	if(f_endBarPresence==0) errorList(10);

	printPHO();

	if(con_Err==0)	fprintf(e, "\n PHO file builded successfully with no error!\n\n");
	  else		fprintf(e, "\n\n PHO file builded with %d error(s)\n\n", con_Err);

	for(int i=0; i<20; i++) 
		printf("-%d-%d-%d-\n", ftNote[i][0], ftNote[i][1], ftNote[i][2]);

	for(int i=0; i<20; i++) {
		for(int j=0; j<8; j++) {
			printf("-%s-", Lyr[i][j]);
		}
		if(!(i%5==0)) printf("   "); else printf("\n");
	}
	printf("##%d##", conCon);
}
