#ifndef __eqclass_h
#define __eqclass_h

#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include "calcdb.h"
using namespace std;

template<class T>
struct printout: public unary_function<T, void>{
   void operator() (T *x){ cout << *x; }
   void operator() (T x){ cout << x; }
};
 
class Eqnode{
public:
   int val;
   int pos;
   int sup;

   Eqnode(int v, int p, int s);

   friend ostream & operator<<(ostream& ostr, Eqnode& eqn){
     ostr << eqn.val << " " << eqn.pos << " " << eqn.sup;
     return ostr;
   }
};


class Eqclass{
 private:
  vector<int> _prefix;
  list<Eqnode> _nodelist;
  
 public:
  Eqclass();
  ~Eqclass();
  
  vector<int> &prefix(){ return _prefix; }
  list<Eqnode> &nlist(){ return _nodelist; }
  int child_of (int pos);
  void add_node(int val, int pos, int sup=0);
  int item(int n);
  void set_prefix(vector<int> &pref, Eqnode &node);
  int get_scope(int pos, int &scnt); 
  void print(Dbase_Ctrl_Blk *DCB);
  friend ostream & operator<<(ostream& ostr, Eqclass& eq);
};

#endif
 
