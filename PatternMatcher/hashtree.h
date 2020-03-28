#ifndef __mhashtree_h
#define __mhashtree_h

#include <map> 
#include <ext/hash_map>
#include <list>
#include <vector>
#include <functional>
#include "eqclass.h"

#define HASHNS __gnu_cxx
using namespace std;

#define DEF_THRESHOLD 1

template<class T>
struct delnode: public unary_function<T, void>{
   void operator() (T x){ delete x; }
};


class HashTree; //forward declaration
typedef map<int, HashTree *, less<int> > HTable;
typedef HTable::value_type HTPair;

class HashTree{
private:
   HTable *theHTable;
   list<Eqclass *> *theEList;
   
   int theFlg;
   int theDepth;   
   bool theLeaf;
   static int theCount;
   static int theThreshold;   
   static int theMaxDepth;
   
   void rehash(); // procedure for converting leaf node to a interior node
public:
   HashTree(int depth=0);
   ~HashTree();
   
   HashTree * hash(int); // procedure to find out which node item hashes to

   void add_element(Eqclass *);
   int isleaf(){ return theLeaf;}
   int& flag(){ return theFlg; }
   int& depth(){return theDepth;}
   bool isempty(){ return (theCount == 0)?true:false; }
   
   HTable &htable(){return *theHTable;}


   list<Eqclass *> *& eqlist(){ return theEList; }
   static int& count() { return theCount;}
   static int& threshold(){ return theThreshold;}
   static int& maxdepth(){ return theMaxDepth;}

   friend ostream & operator<<(ostream& fout, HashTree& ht);
};


//for pruning candidate subtrees
typedef HASHNS::hash_multimap<int, vector<int> *, HASHNS::hash<int>, equal_to<int> > cHTable;
typedef pair<cHTable::iterator, cHTable::iterator> cHTFind;
typedef cHTable::value_type cHTPair;
#define FHTSIZE 100 
class FreqHT: public cHTable{
public:
   FreqHT(int sz=FHTSIZE): cHTable(sz){}
   ~FreqHT(){ clearall(); }

   void clearall(){
      cHTable::iterator hi = begin();
      for (; hi != end(); hi++){
         delete (*hi).second;
      }
      clear();
   }
   
   void add(Eqclass *eq);
   bool find(vector<int> &cand, int hval);
};

#endif
