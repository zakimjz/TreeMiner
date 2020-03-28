#ifndef __STATS_H
#define __STATS_H

#include<iostream>
#include<vector>

using namespace std;

class iterstat{
public:
   iterstat(int nc=0, int nl=0, double tt=0.0,double aa=0.0):
      numcand(nc),numlarge(nl),time(tt),avgtid(aa){}
   int numcand;
   int numlarge;
   double time;
   double avgtid;
};
 
class Stats: public vector<iterstat *>{
public:
   static double sumtime;
   static int sumcand;
   static int sumlarge;
   static double tottime;

   Stats();

   void add(iterstat *is);
   void add(int cand, int freq, double time, double avgtid=0.0);

   friend ostream& operator << (ostream& fout, Stats& stats);
};

#endif
