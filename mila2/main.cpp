/* main.c */
/* syntakticky analyzator */

#include "lexan.h"
#include "parser.h"
#include "strom.h"
#include "zaspoc.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
   char *jmeno;
   printf("Syntakticky analyzator\n");
   if (argc == 1) {
      printf("Vstup z klavesnice, zadejte zdrojovy text\n");
      jmeno = NULL;
   } else {
      jmeno = argv[1];
      printf("Vstupni soubor %s\n", jmeno);
   }
   InitLexan(jmeno);
   CtiSymb();
   Prog *prog = Program();
   prog = (Prog*)(prog->Optimize());
   prog->Translate();
   Print();
   Run();
   printf("Konec\n");
}
