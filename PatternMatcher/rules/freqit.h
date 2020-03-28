#ifndef __FREQ__
#define __FREQ__

#include <vector>
#include <iostream>

using namespace std;

class FreqIt: public vector<int>{
   friend ostream& operator << (ostream& fout, FreqIt& freq);
 public:
   FreqIt():fsup(0), dbsup(0), fratio(0.0), dbratio(0.0){}
   
   int fsup; //frequency in the freq file
   int dbsup; //frequency in the db file
   double fratio; //ratio in freq and db file
   double dbratio;
};

typedef vector<FreqIt *> FreqAry;

#endif
