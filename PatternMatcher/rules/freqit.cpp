#include "freqit.h"

ostream& operator << (ostream& fout, FreqIt& freq)
{
   int i;
   for (i=0 ; i < freq.size(); ++i){
      fout << freq[i] << " ";
   }
   
   fout << "- " << freq.fsup << " " << freq.dbsup;
   fout << " " << freq.fratio << " " << freq.dbratio;
   

   return fout;
}
