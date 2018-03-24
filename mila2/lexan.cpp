/* lexan.cpp */

#include "lexan.h"
#include "vstup.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

LexSymbol Symb;
char      Ident[MaxLenIdent];  /* atribut symbolu IDENT */
int       Cislo;               /* atribut symbolu NUMB */

static int Znak;   // vstupni znak
static char Vstup; // vstupni symbol

void CtiVstup(void)
{
   Znak = CtiZnak();
   if ((Znak>='A' && Znak<='Z') || (Znak>='a' && Znak<='z'))
      Vstup = 'p';
   else if (Znak>='0' && Znak<='9')
      Vstup = 'c';
   else if (Znak == EOF)
      Vstup = 'e';
   else if (Znak <= ' ')
      Vstup = 'm';
   else
      Vstup = Znak;
}

const struct {const char* slovo; LexSymbol symb;} TabKS[] = {
   {"var", kwVAR},
   {"const", kwCONST},
   {"begin", kwBEGIN},
   {"end", kwEND},
   {"if", kwIF},
   {"then", kwTHEN},
   {"else", kwELSE},
   {"while", kwWHILE},
   {"do", kwDO},
   {"write", kwWRITE},
   {NULL, EOI}
};

LexSymbol KlicoveSlovo(char* id)
{
   int i = 0;
   while (TabKS[i].slovo) 
      if (strcmp(id, TabKS[i].slovo)==0) 
         return TabKS[i].symb;
      else
         i++;
   return IDENT;
}

void Chyba(const char* text)
{
   printf("\n%s\n", text);
   exit(1);
}

const char *LexSymboly[] = {
   "ident", "cislo", "+", "-", "*", "/",
   "=", "<>", "<", ">", "<=", ">=", "(", ")", ":=",
   ",", ";",
   "var", "const", "begin", "end", "if", "then", "else",
   "while", "do", "write"
};

void ChybaSrovnani(LexSymbol s)
{
   printf("chyba pri srovnani, ocekava se %s\n", LexSymboly[s]);
   exit(1);
}

void CtiSymb(void)
{
   int delkaId;
q0: 
   while (Vstup=='m') CtiVstup();
   switch (Vstup) {
   case 'e':
      Symb = EOI;
      break;
   case '{':
      CtiVstup();
      while (Vstup!='}' && Vstup!='e')
         CtiVstup();
      if (Vstup=='e')
         Chyba("neocekavany konec souboru v komentari");
      else {
         CtiVstup();
         goto q0;
      }
   case 'p':
      delkaId = 1; Ident[0] = Znak;
      CtiVstup();
      while (Vstup=='p' || Vstup=='c') {
         Ident[delkaId] = Znak; delkaId++;
         CtiVstup();
      }
      Ident[delkaId] = 0;
      Symb = KlicoveSlovo(Ident);
      break;
   case 'c':
      Cislo = Znak - '0';
      CtiVstup();
      while (Vstup=='c') {
         Cislo = 10 * Cislo + Znak - '0';
         CtiVstup();
      }
      Symb = NUMB;
      break;
   case ',':
      Symb = COMMA;
      CtiVstup();
      break;
   case ';':
      Symb = SEMICOLON;
      CtiVstup();
      break;
   case '+':
      Symb = PLUS;
      CtiVstup();
      break;
   case '-':
      Symb = MINUS;
      CtiVstup();
      break;
   case '*':
      Symb = TIMES;
      CtiVstup();
      break;
   case '/':
      Symb = DIVIDE;
      CtiVstup();
      break;
   case '=':
      Symb = EQ;
      CtiVstup();
      break;
   case '<':
      CtiVstup();
      switch (Vstup) {
      case '>': 
         Symb = NEQ;
         CtiVstup();
         break;
      case '=':
         Symb = LTE;
         CtiVstup();
         break;
      default:
         Symb = LT;
      }
      break;
   case '>':
      CtiVstup();
      if (Vstup=='=') {
         Symb = GTE;
         CtiVstup();
      } else
         Symb = GT;
      break;
   case '(':
      Symb = LPAR;
      CtiVstup();
      break;
   case ')':
      Symb = RPAR;
      CtiVstup();
      break;
   case ':':
      CtiVstup();
      if (Vstup=='=') {
         Symb = ASSGN;
         CtiVstup();
         break;
      } 
   default:
      Chyba("nedovoleny znak");
   }
}

void InitLexan(char *jmeno)
{
   InitVstup(jmeno);
   CtiVstup();
}
