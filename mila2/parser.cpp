/* parser.cpp */

#include "strom.h"
#include "parser.h"
#include "lexan.h"
#include "tabsym.h"
#include <stdio.h>
#include <string.h>

Prog *Program();
void Dekl();
void DeklKonst();
void ZbDeklKonst();
void DeklProm();
void ZbDeklProm();
StatmList *SlozPrikaz();
StatmList *ZbPrikazu();
Statm *Prikaz();
Statm *CastElse();
Expr *Podminka();
Operator RelOp();
Expr *Vyraz();
Expr *ZbVyrazu(Expr*);
Expr *Term();
Expr *ZbTermu(Expr*);
Expr *Faktor();

void Srovnani(LexSymbol s)
{
   if (Symb == s)
      CtiSymb();
   else
      ChybaSrovnani(s);
}

void Srovnani_IDENT(char *id)
{
   if (Symb == IDENT) {
      strcpy(id, Ident);
      CtiSymb();
   } else
      ChybaSrovnani(IDENT);
}

void Srovnani_NUMB(int *h)
{
   if (Symb == NUMB) {
      *h = Cislo;
      CtiSymb();
   } else 
      ChybaSrovnani(NUMB);
}

Prog *Program()
{
   Dekl();
   return new Prog(SlozPrikaz());
}

void Dekl()
{
   switch (Symb) {
   case kwVAR:
      DeklProm();
      Dekl();
      break;
   case kwCONST:
      DeklKonst();
      Dekl();
      break;
   default:
      ;
   }
}

void DeklKonst()
{
   char id[MaxLenIdent];
   int hod;
   CtiSymb();
   Srovnani_IDENT(id);
   Srovnani(EQ);
   Srovnani_NUMB(&hod);
   deklKonst(id, hod);
   ZbDeklKonst();
   Srovnani(SEMICOLON);
}

void ZbDeklKonst()
{
   if (Symb == COMMA) {
      char id[MaxLenIdent];
      int hod;
      CtiSymb();
      Srovnani_IDENT(id);
      Srovnani(EQ);
      Srovnani_NUMB(&hod);
      deklKonst(id, hod);
      ZbDeklKonst();
   }
}
   
void DeklProm()
{
   char id[MaxLenIdent];
   CtiSymb();
   Srovnani_IDENT(id);
   deklProm(id);
   ZbDeklProm();
   Srovnani(SEMICOLON);
}

void ZbDeklProm()
{
   if (Symb == COMMA) {
      char id[MaxLenIdent];
      CtiSymb();
      Srovnani_IDENT(id);
      deklProm(id);
      ZbDeklProm();
   }
}

StatmList *SlozPrikaz()
{
   Srovnani(kwBEGIN);
   Statm *p = Prikaz();
   StatmList *su = new StatmList(p, ZbPrikazu());
   Srovnani(kwEND);
   return su;
}

StatmList *ZbPrikazu()
{
   if (Symb == SEMICOLON) {
      CtiSymb();
      Statm *p = Prikaz();
      return new StatmList(p, ZbPrikazu());
   }
   return 0;
}

Statm *Prikaz()
{
   switch (Symb) {
   case IDENT: {
      Var *var = new Var(adrProm(Ident),false);
      CtiSymb();
      Srovnani(ASSGN);
      return new Assign(var, Vyraz());
   }
   case kwWRITE:
      CtiSymb();
      return new Write(Vyraz());
   case kwIF: {
      CtiSymb();
      Expr *cond = Podminka();
      Srovnani(kwTHEN);
      Statm *prikaz = Prikaz();
      return new If(cond, prikaz, CastElse());
   }
   case kwWHILE: {
      Expr *cond;
      CtiSymb();
      cond = Podminka();
      Srovnani(kwDO);
      return new While(cond, Prikaz());
   }
   case kwBEGIN:
      return SlozPrikaz();
   default:
      return new Empty;
   }
}

Statm *CastElse()
{
   if (Symb == kwELSE) {
      CtiSymb();
      return Prikaz();
   }
   return 0;
}

Expr *Podminka()
{
   Expr *left = Vyraz();
   Operator op = RelOp();
   Expr *right = Vyraz();
   return new Bop(op, left, right);
}

Operator RelOp()
{
   switch (Symb) {
   case EQ:
      CtiSymb();
      return Eq;
   case NEQ:
      CtiSymb();
      return NotEq;
   case LT:
      CtiSymb();
      return Less;
   case GT:
      CtiSymb();
      return Greater;
   case LTE:
      CtiSymb();
      return LessOrEq;
   case GTE:
      CtiSymb();
      return GreaterOrEq;
   default:
      Chyba("neocekavany symbol");
      return Error;
   }
}

Expr *Vyraz()
{
   if (Symb == MINUS) {
      CtiSymb();
      return ZbVyrazu(new UnMinus(Term()));
   }
   return ZbVyrazu(Term());
}

Expr *ZbVyrazu(Expr *du)
{
   switch (Symb) {
   case PLUS:
      CtiSymb();
      return ZbVyrazu(new Bop(Plus, du, Term()));
   case MINUS:
      CtiSymb();
      return ZbVyrazu(new Bop(Minus, du, Term()));
   default:
      return du;
   }
}

Expr *Term()
{
   return ZbTermu(Faktor());
}

Expr *ZbTermu(Expr *du)
{
   switch (Symb) {
   case TIMES:
      CtiSymb();
      return ZbTermu(new Bop(Times, du, Faktor()));
   case DIVIDE:
      CtiSymb();
      return ZbTermu(new Bop(Divide, du, Faktor()));
   default:
      return du;
   }
}

Expr *Faktor()
{
   switch (Symb) {
   case IDENT:
      char id[MaxLenIdent];
      Srovnani_IDENT(id);
      return VarOrConst(id);
   case NUMB:
      int hodn;
      Srovnani_NUMB(&hodn);
      return new Numb(hodn);
   case LPAR: {
      CtiSymb();
      Expr *su = Vyraz();
      Srovnani(RPAR);
      return su;
   }
   default:
      Chyba("neocekavany symbol");
      return 0;
   }
}
