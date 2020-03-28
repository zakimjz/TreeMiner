#include <stack>
#include <algorithm>  
#include "eqclass.h"


ostream & operator<<(ostream& ostr, Eqnode& eqn){
   //ostr << fidx[eqn.val] << " (" << eqn.pos << ") - " << eqn.sup << endl;
   ostr << eqn.val << " - " << eqn.sup << endl;
   //ostr << eqn.tlist;
   return ostr;
}

