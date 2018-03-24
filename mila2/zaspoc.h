/* zaspoc.h */

#ifndef _ZASPOC_
#define _ZASPOC_

enum TypInstr {
   TA, TC, BOP, UNM, DR, ST, IFJ, JU, WRT, DUP, STOP
};
enum Operator {
   Plus, Minus, Times, Divide,
   Eq, NotEq, Less, Greater, LessOrEq, GreaterOrEq,
   Error
};

int Gener(TypInstr, int = 0);
void GenTR(char*);
void PutIC(int);
int GetIC();
void Print();
void Run();

#endif
