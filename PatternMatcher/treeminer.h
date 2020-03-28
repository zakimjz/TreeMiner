#ifndef __treeminer_H
#define __treeminer_H
 
#include <vector>

using namespace std;

#define BranchIt -1 //-1 indicates a branch in the tree

//enums
enum sort_vals {nosort, incr, decr};
enum prune_vals {noprune, prune};

//stack node class used in count_support()
class stknode{
public:
  int ts;//trans scope
  int ti;//trans idx
  int pi;//prefix idx
  stknode(int tss, int tii, int pii){
    ts = tss;
    ti = tii;
    pi = pii;
  }
};

extern double MINSUP_PER;
extern int MINSUPPORT;
extern int DBASE_MAXITEM;
extern int DBASE_NUM_TRANS;

extern bool output;
extern sort_vals sort_type;
extern prune_vals prune_type;
extern ostream & operator<<(ostream& fout, vector<int> &vec);

#endif
