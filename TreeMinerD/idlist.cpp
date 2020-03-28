#include "idlist.h"

ostream & operator<<(ostream& ostr, ival& idn){
   ostr << "[" << idn.lb << ", " << idn.ub << "]";
   return ostr;
}

ostream & operator<<(ostream& ostr, idnode& idn){
   ostr << idn._cid << " ";
   ostr << idn._scope << " -- ";
   for(int i = 0; i < idn._pslist.size(); ++i)
      ostr << idn._pslist[i] << ", ";
   return ostr;
}         

ostream & operator<<(ostream& fout, idlist& idl)
{
   for (int j=0; j < idl.size(); j++){
      fout << "\t" << idl[j] << endl;
   }
   return fout;
}

ostream & operator<<(ostream& fout, vector<int> &vec){
  for (int i=0; i < vec.size(); i++)
     fout << vec[i] << " ";
  return fout;
}

