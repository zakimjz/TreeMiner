#ifndef __eqclass_h
#define __eqclass_h

#include <iostream>
#include <list>
#include <vector>

#include "countfreq.h"
#include "calcdb.h"
#include "idlist.h"

using namespace std;

class Eqnode{
public:
   int val; //item value
   int pos; //which node is it connected to in parent prefix
   int sup; //support
   idlist tlist; //scope list for this item
   
   Eqnode(int v, int p, int s=0): val(v), pos(p), sup(s){}

   static bool supcmp (Eqnode *n1, Eqnode *n2){
      bool res = false;
      if ((n1)->sup < (n2)->sup) res = true;
      
      return res;
   }
   friend ostream & operator<<(ostream& ostr, Eqnode& eqn);
};


#endif
 
