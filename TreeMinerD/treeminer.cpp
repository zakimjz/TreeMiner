#include<iostream>
#include <unistd.h>
#include <algorithm>
#include <stdio.h>
//#include <stl.h>
#include <list>

//headers
#include "treeminer.h"
#include "timetrack.h"
#include "calcdb.h"
#include "eqclass.h"
#include "stats.h"
#include "hashtable.h"

using namespace std;

//global vars
char infile[300];
Dbase_Ctrl_Blk *DCB;
Stats stats;

double MINSUP_PER;
int MINSUPPORT=-1;
int DBASE_MAXITEM;
int DBASE_NUM_TRANS;

//default flags
bool output = false; //don't print freq subtrees
bool output_idlist = false; //don't print idlist
bool count_unique = true; //count support only once per tree
bool use_fullpath = false; //use reduced scope to conserve mem

sort_vals sort_type = incr; //default is to sort in increasing order
alg_vals alg_type = treeminer; //default is to find all freq patterns

prune_vals prune_type = noprune; //prune
int prune_value;
int prune_cnt;
FreqHT FK; //to store freq subtrees for pruning

vector<int> *ITCNT = NULL; //used for sorting F1
bool F1cmp(int x, int y){
   bool res = false;
   if ((*ITCNT)[x] < (*ITCNT)[y]) res = true;

   if (sort_type == incr) return res;
   else return !res;
}
   

void parse_args(int argc, char **argv)
{
   extern char * optarg;
   int c;

   if (argc < 2)
     cout << "usage: treeminer -i<infile> -s <support>\n";
   else{
     while ((c=getopt(argc,argv,"a:bfi:lop:P:Q:s:S:uz:"))!=-1){
       switch(c){
       case 'a':
          alg_type = (alg_vals) atoi(optarg);
          break;
       case 'b':
          Dbase_Ctrl_Blk::binary_input= true;
          break;
        case 'f':
          use_fullpath = true;
          break;
       case 'i': //input files
	 sprintf(infile,"%s",optarg);
	 break;
       case 'l': //print idlists along with freq subtrees
          output=true;
          output_idlist = true;
          break;
       case 'o': //print freq subtrees
	 output = true;
	 break;
       case 'p':
          prune_type = (prune_vals) atoi(optarg);
          break;
       case 'P': //for use with itemprune; prune pattern if solely of
                 //this item
          prune_value = atoi(optarg);
          break;
       case 'Q': //for use with itemprune; prune pattern if solely of
                 //this item occurs more than prune_cnt times
          prune_cnt = atoi(optarg);
          break;
       case 's': //support value for L2
	 MINSUP_PER = atof(optarg);
	 break;
       case 'S': //absolute support
	 MINSUPPORT = atoi(optarg);
	 break;
       case 'u': //count support multiple times per tree
	 count_unique = false;
	 break;
       case 'z':
          sort_type = (sort_vals) atoi(optarg);
          break;
       }               
     }
   }
}

void get_F1()
{
  TimeTracker tt;
  double te;

  int i, j, it;
  const int arysz = 10;
  
  vector<int> itcnt(arysz,0); //count item frequency
  vector<int> flgs(arysz,-1);

  tt.Start();

  DBASE_MAXITEM=0;
  DBASE_NUM_TRANS = 0;
  
   while(DCB->get_next_trans())
   {
      //cout << "TRANS " << DCB->Cid << " " << DCB->Tid
      //     << " " << DCB->TransSz << " -- ";
      //for (i=0; i < DCB->TransSz; i++)
      //   cout << " " << DCB->TransAry[i];
      //cout << endl;
      
      for (i=0; i < DCB->TransSz; i++){
         it = DCB->TransAry[i];
         if (it == BranchIt) continue;
         
         if (it >= DBASE_MAXITEM){
            itcnt.resize(it+1,0);
            flgs.resize(it+1,-1);
            DBASE_MAXITEM = it+1;
            //cout << "IT " << DBASE_MAXITEM << endl;
         }
         
         if (count_unique){
            if(flgs[it] == DCB->Cid) continue;
            else flgs[it] = DCB->Cid;
         }
         itcnt[it]++;
      }
      
      if (DCB->MaxTransSz < DCB->TransSz) DCB->MaxTransSz = DCB->TransSz;     
      DBASE_NUM_TRANS++;
   }
   
   //for (i=0; i < DCB->TransSz; i++){
   //   it = DCB->TransAry[i];
   //   if (it != BranchIt){
   //      cout << it << " " << itcnt[it] << endl;
   //   }
   //}

   //set the value of MINSUPPORT
   if (MINSUPPORT == -1)
     MINSUPPORT = (int) (MINSUP_PER*DBASE_NUM_TRANS+0.5);
   
   if (MINSUPPORT<1) MINSUPPORT=1;
   cout<<"DBASE_NUM_TRANS : "<< DBASE_NUM_TRANS << endl;
   cout<<"DBASE_MAXITEM : "<< DBASE_MAXITEM << endl;
   cout<<"MINSUPPORT : "<< MINSUPPORT << " (" << MINSUP_PER << ")" << endl;

   //count number of frequent items
   DCB->NumF1 = 0;
   for (i=0; i < DBASE_MAXITEM; i++)
     if (itcnt[i] >= MINSUPPORT)
       DCB->NumF1++;

   int *it_order = new int[DBASE_MAXITEM];
   for (i=0; i < DBASE_MAXITEM; i++)
      it_order[i] = i;
   
   if (sort_type != nosort){
      ITCNT = &itcnt;
      sort(&it_order[0], &it_order[DBASE_MAXITEM], F1cmp);
   }

   //construct forward and reverse mapping from items to freq items
   DCB->FreqIdx = new int [DCB->NumF1];
   DCB->FreqMap = new int [DBASE_MAXITEM];
   for (i=0,j=0; i < DBASE_MAXITEM; i++) {
      if (itcnt[it_order[i]] >= MINSUPPORT) {
         //if (output) cout << i << " " << it_order[i] 
         //                 << " - " << itcnt[it_order[i]] << endl;
         if (output) cout << it_order[i] << " - " << itcnt[it_order[i]] << endl;
         DCB->FreqIdx[j] = it_order[i];
         DCB->FreqMap[it_order[i]] = j;
         j++;
      }
      else DCB->FreqMap[it_order[i]] = -1;
   }
   
   //cout<< "F1 - " << DCB->NumF1 << " " << DBASE_MAXITEM << endl;  
   
   if (sort_type != nosort){
      ITCNT = NULL;
      delete [] it_order;
   }
   
   te = tt.Stop();
   stats.add(DBASE_MAXITEM, DCB->NumF1, te);

}

list<Eqclass *> * get_F2()
{
  int i,j;
  int it1, it2;
  int scnt;
  
  TimeTracker tt;
  double te;

  tt.Start();

  list<Eqclass *> *F2list = new list<Eqclass *>;

  //itcnt2 is a matrix of pairs p, p.first is count, p.second is flag
  int **itcnt2 = new int*[DCB->NumF1];
  int **flgs = new int*[DCB->NumF1];
  //unsigned int **itcnt2 = new unsigned int *[DCB->NumF1];
  for (i=0; i < DCB->NumF1; i++){
    itcnt2[i] = new int [DCB->NumF1];
    flgs[i] = new int [DCB->NumF1];
    //cout << "alloc " << i << " " << itcnt2[i] << endl;
    for (j=0; j < DCB->NumF1; j++){
      itcnt2[i][j] = 0;
      flgs[i][j] = -1;
    }
  }
    
   DCB->alloc_idlists();
   
   while(DCB->get_next_trans())
   {
      DCB->get_valid_trans();
      DCB->make_vertical();
      //DCB->print_trans();
      //count a pair only once per cid
      for (i=0; i < DCB->TransSz; i++){
         it1 = DCB->TransAry[i];
         if (it1 != BranchIt){
            scnt = 0;
            for (j=i+1; scnt >= 0 && j < DCB->TransSz; j++){
               it2 = DCB->TransAry[j];
               if (it2 != BranchIt){
		 scnt++;
		 if (count_unique){
		   if (flgs[it1][it2] == DCB->Cid) continue;
		   else flgs[it1][it2] = DCB->Cid;
		 }
		 //cout << "cnt " << it1 << " " << it2 << endl;
		 itcnt2[it1][it2]++;
               }
               else scnt--;
            }
         }
      }
   }                           
   
   int F2cnt=0;

   // count frequent patterns and generate eqclass
   Eqclass *eq;
   for (i=0; i < DCB->NumF1; i++) {
      eq = NULL;
      for (j=0; j < DCB->NumF1; j++) {
	//cout << "access " << i << " " << j << endl;
         if (itcnt2[i][j] >= MINSUPPORT){
            F2cnt++;
            if (eq == NULL){
               eq = new Eqclass;
               eq->prefix().push_back(i);
            }
	    eq->add_node(j,0);
            
	    //if (output) 
	    //  cout << DCB->FreqIdx[i] << " " << DCB->FreqIdx[j] 
            //   << " - " << itcnt2[i][j] << endl;
         }
      }   
      if (eq != NULL) F2list->push_back(eq);
   }
   
   for (i=0; i < DCB->NumF1; i++) {
     //cout << "dealloc " << i << " " << itcnt2[i] << endl;
     delete [] itcnt2[i];
     //cout << "dealloc " << i << " " << flgs[i] << endl;
     delete [] flgs[i];
   }

   delete [] itcnt2;
   delete [] flgs;
   
   
   //cout << "F2 - " << F2cnt << " " << DCB->NumF1 * DCB->NumF1 << endl;
   te = tt.Stop();
   stats.add(DCB->NumF1 * DCB->NumF1, F2cnt, te);

   return F2list;
}

static bool notfrequent (Eqnode &n){
  //cout << "IN FREQ " << n.sup << endl;
  if (n.sup >= MINSUPPORT) return false;
  else return true;
}


void old_check_ins(idlist *l1, idlist *l2, Eqnode *ins, 
                int st1, int st2, int en1, int en2){

   //after modification, this routine doesn't work for multiple
   //occurrence counting in the same tree
   idnode *node=NULL;
   ival_type cmpval;
   idnode *n1node, *n2node;
   ival n1, n2;
   
   bool found_flg = false;
   int nearpos; //closest parent
   
   //for each ival in n2, find closest parent in n1
   //cout << "bounds " << st1 << " " << en1 << " " << st2 << " " << en2 << endl;
   for (int pos2=st2; pos2 < en2; ++pos2){
      n2node = &(*l2)[pos2];
      n2 = n2node->scope();
      for (int pos1 = st1; pos1 < en1; ++pos1){
         n1node = &(*l1)[pos1];
         n1 = n1node->scope();
         cmpval = ival::compare(n2, n1);
         //cout << "COMPARE " << cmpval << " : " << n2 << " -- " << n1 << endl;
         if (cmpval == sub){ //n2 under n1
            nearpos = pos1;
            //find closest parent
            found_flg = true;
            for (int pos = pos1+1; pos < en1; ++pos){
               n1node = &(*l1)[pos];
               ival n = n1node->scope();
               cmpval = ival::compare(n2, n);
               if (cmpval == sub){
                  nearpos = pos;
               }
               else if (cmpval == after) break;
            }
            
            n1node = &(*l1)[nearpos];
            //cmpval = ival::compare((*l1)[nearpos].front(), n2node->back());
            //if (cmpval == equals){
            //   node = new idnode(*n2node);
            //   node->insert(node->begin()+(node->size()-1), 
            //                (*l1)[nearpos].back()); 
            // }
            //else{
            node = new idnode(n2node->cid(), n2node->scope());
            //if (ival::compare(n1node->scope(), 
            //                  n2node->prefixlist().back()) == equals){
            //   node->add_scope(n1node->prefixlist().back(),
            //                   n2node->prefixlist());
            //}
            //else{
            node->add_scope(n1node->prefixlist(), n1node->scope());
            //}
            ins->tlist.push_back(*node);
            break; //go to next n2
         }
         //else if (cmpval == after){
         //   // the moment n2 comes after n1, we are done 
         //   break;
         //}
      }
   }

   if (found_flg && count_unique) ins->sup++;
}

void check_ins(idlist *l1, idlist *l2, Eqnode *ins, 
                int st1, int st2, int en1, int en2){

   //after modification, this routine doesn't work for multiple
   //occurrence counting in the same tree
   idnode *node=NULL;
   ival_type cmpval;
   idnode *n1node, *n2node;
   ival n1, n2;
   
   bool found_flg = false;
   int nearpos; //closest parent
   
   //for each ival in n2, find closest parent in n1
   //cout << "bounds " << st1 << " " << en1 << " " << st2 << " " << en2 << endl;
   for (int pos2=st2; pos2 < en2; ++pos2){
      n2node = &(*l2)[pos2];
      n2 = n2node->scope();
      bool flg = false;
      //find closest parent
      for (int pos1 = st1; pos1 < en1; ++pos1){
         n1node = &(*l1)[pos1];
         n1 = n1node->scope();
         cmpval = ival::compare(n2, n1);
         //cout << "COMPARE " << cmpval << " : " << n2 << " -- " << n1 << endl;
         if (cmpval == sub){ //n2 under n1
            nearpos = pos1;
            flg = true;
         }   
         else if (cmpval != after) break;
      }
      
      if (flg){
         found_flg = true;
         n1node = &(*l1)[nearpos];
         //cmpval = ival::compare((*l1)[nearpos].front(), n2node->back());
         //if (cmpval == equals){
         //   node = new idnode(*n2node);
         //   node->insert(node->begin()+(node->size()-1), 
         //                (*l1)[nearpos].back()); 
         // }
         //else{
         node = new idnode(n2node->cid(), n2node->scope());
         //if (ival::compare(n1node->scope(), 
         //                  n2node->prefixlist().back()) == equals){
         //   node->add_scope(n1node->prefixlist().back(),
         //                   n2node->prefixlist());
         //}
         //else{
         node->add_scope(n1node->prefixlist(), n1node->scope());
         //}
         ins->tlist.push_back(*node);
      }
      //else if (cmpval == after){
      //   // the moment n2 comes after n1, we are done 
      //   break;
      //}
   }

   if (found_flg && count_unique) ins->sup++;
}
   
void old_check_outs(idlist *l1, idlist *l2, Eqnode *outs,
                int st1, int st2, int en1, int en2, int outs_pos){
   //after modification, this routine doesn't work for multiple
   //occurrence counting in the same tree
   idnode *n1node, *n2node, *nnode, *node=NULL;
   ival_type cmpval;
   ival n1, n2, n;
   
   int nearpos; //closest parent
   
   bool found_flg = false;
   //for each ival in n2, find closest parent in n1
   for (int pos2=st2; pos2 < en2; ++pos2){
      n2node = &(*l2)[pos2];
      n2 = n2node->scope();
      for (int pos1 = st1; pos1 < en1; ++pos1){
         n1node = &(*l1)[pos1];
         n1 = n1node->scope();
         cmpval = ival::compare(n2, n1);
         //cout << "OUT POS " << outs_pos << endl;
         //cout << "oCOMPARE " << cmpval << " : " << n2val 
         //     << " -- " << n1val << endl;
         if (cmpval == after){ //n2 after n1
            bool flg = false;
            if (n2node->path_contains(*n1node, outs_pos)){
               //cout << "PATH EQUAL\n";
               if (n1node->get_ival(outs_pos+1, n)){
                  cmpval = ival::compare(n2, n);
                  if (cmpval == after) flg = true;
               }
               else flg = true;
            }
            if (flg){               
               found_flg = true;
               node = new idnode(n2node->cid(), n2node->scope());
               node->add_scope(n1node->prefixlist(), outs_pos);
               outs->tlist.push_back(*node);
               break; //go to next n2
            }
         }
         //else if (cmpval == sup){
         //   //the moment n2 becomes a superset of n1, we are done
         //   break;
         //}
      }
   }

   if (found_flg && count_unique) outs->sup++;
}

void check_outs(idlist *l1, idlist *l2, Eqnode *outs,
                int st1, int st2, int en1, int en2, int outs_pos){
   //after modification, this routine doesn't work for multiple
   //occurrence counting in the same tree
   idnode *n1node, *n2node, *nnode, *node=NULL;
   ival_type cmpval;
   ival n1, n2, n, nx;
   
   int nearpos; //closest parent
   
   bool found_flg = false;
   //for each ival in n2, find closest parent in n1
   for (int pos2=st2; pos2 < en2; ++pos2){
      n2node = &(*l2)[pos2];
      n2 = n2node->scope();
      bool flg = false;
      for (int pos1 = st1; pos1 < en1; ++pos1){
         n1node = &(*l1)[pos1];
         n1 = n1node->scope();
         cmpval = ival::compare(n2, n1);
         //cout << "OUT POS " << outs_pos << endl;
         //cout << "oCOMPARE " << cmpval << " : " << n2val 
         //     << " -- " << n1val << endl;
         if (cmpval == after){ //n2 after n1
            //make sure that n2 is under n1's prefix at pos outs_pos
            n1node->get_ival(outs_pos, n);
            cmpval = ival::compare(n2, n);
            if (cmpval == sub){
               //make sure that n2 is after n1's prefix at outs_pos+1
               if (!n1node->get_ival(outs_pos+1, n) ||
                   ival::compare(n2, n) == after){
                  flg = true;
                  nearpos = pos1;
               }
            }
            else if (cmpval == after){
               n2node->get_ival(outs_pos, nx);
               if (ival::compare(nx,n) == sup){
                  if (!flg){
                     flg = true;
                     nearpos = pos1;
                  }
               }
            }
         }
         else if (cmpval != sub) break;
      }
      
      if (flg){               
         found_flg = true;
         n1node = &(*l1)[nearpos];
         node = new idnode(n2node->cid(), n2node->scope());
         node->add_scope(n1node->prefixlist(), outs_pos);
         outs->tlist.push_back(*node);
      }
   }

   if (found_flg && count_unique) outs->sup++;
}

void get_intersect(idlist *l1, idlist *l2, Eqnode *ins, Eqnode *outs,
                   int outs_pos) 
{
   static idnode *n1, *n2;

   int i1 = 0, i2 = 0;
   int e1, e2;

   //cout << "GISECT:\n";
   //cout << *l1;
   //cout << "****\n";
   //cout << *l2;
   
   while (i1 < l1->size() && i2 < l2->size()){
      n1 = &(*l1)[i1];
      n2 = &(*l2)[i2];

      //look for matching cids
      if (n1->cid() < n2->cid()) i1++;
      else if (n1->cid() > n2->cid()) i2++;
      else{
         //cids match
         e1 = i1;
         e2 = i2;

         //check the cid end positions in it1 and it2
         while (e1 < l1->size() && (*l1)[e1].cid() == n1->cid()) e1++;
         while (e2 < l2->size() && (*l2)[e2].cid() == n2->cid()) e2++;

         //cout << "ISECT " << i1 << " " << i2 << " " << e1 << " " << e2 << endl;
         //increment support if candidate found
         //cout << "INS:\n";
         if (ins) check_ins(l1, l2, ins, i1, i2, e1, e2);
         //cout << "OUTS:\n";
         if (outs) check_outs(l1, l2, outs, i1, i2, e1, e2, outs_pos);

         //restore index to end of cids
         i1 = e1;
         i2 = e2;
      }
   }
}


bool lexsmaller(vector<int> &subtree, vector<int> &cand)
{
   int i,j;

   for (i=0, j=0; i < subtree.size() && j < cand.size();){

      if (subtree[i] > cand[j]){
         if (cand[j] != BranchIt) return false;
         else{
            while (cand[j] == BranchIt) j++;
            if (subtree[i] > cand[j]) return false;
            else if (subtree[i] < cand[j]) return true;
            else return false;
         }
         
      }
      else if (subtree[i] < cand[j]){
         if (subtree[i] != BranchIt) return true;
         else{
            while(subtree[i] == BranchIt) i++;
            if (subtree[i] > cand[j]) return false;
            else if (subtree[i] < cand[j]) return true;
            else return true;
         }
      }
      else{
         i++;
         j++;
      }
   }
   return false;
}


Eqnode *test_node(int iter, Eqclass *eq, int val, int pos)
{
   Eqnode *eqn = NULL;
   static vector<int> cand;
   static vector<int> subtree;
 
   int hval;
   int scope, scnt;
   int i,j,k;
   
   //if noprune, return with a new Eqnode
   if (prune_type == noprune){
      eqn = new Eqnode(val,pos);
      return eqn;
   }
   
   //perform pruning
   if (prune_type == itemprune){
      int patsize = 0;
      int itcnt = 0;
      cand = eq->prefix();
      for (i = 0; i < cand.size(); ++i){
         if (cand[i] != BranchIt){
            patsize++;
            if (cand[i] == prune_value) itcnt++;
         }
      }
      if ((itcnt == patsize && patsize >= 2) || itcnt >= prune_cnt){
         return eqn;
      }
      else{
         eqn = new Eqnode(val,pos);
         return eqn;
      }
   }
   //prune based on frequent subtree

   //form the candidate preifx
   cand = eq->prefix();
   scnt = eq->get_scope(pos, scope); //what is the scope of node.pos

   while(scnt > scope){
      cand.push_back(BranchIt);
      scnt--;
   }
   cand.push_back(val);

   //check subtrees
   int omita, omitb;
   bool res = true;
   //omit i-th item (omita) and associated BranchIt (omitb)

   for (i=iter-3; i >= 0; i--){
      //find pos for i-th item
      for (j=0,k=0; j < cand.size(); j++){
         if (cand[j] != BranchIt){
            if (k == i){
               omita = j;
               break;
            }
            else k++;
         }
      }
      
      //find pos for associated BranchIt
      scnt = 0;
      for(j++; j < cand.size() && scnt >= 0; j++){
         if (cand[j] == BranchIt) scnt--;
         else scnt++;
      }
      if (scnt >= 0) omitb = cand.size();
      else omitb = j-1;

      //cout << "OMIT " << i << " " << omita << " " << omitb << endl;

      hval = 0;
      subtree.clear();
      bool rootless = false;
      scnt = 0;
      for (k=0; k < cand.size() && !rootless; k++){
         if (k != omita && k != omitb){
            subtree.push_back(cand[k]);
            if (cand[k] != BranchIt){
               hval += cand[k];
               scnt++;
            }
            else scnt--;
            if (scnt <= 0) rootless = true;
            
         }
      }

      //cout << "LEXTEST " << subtree << " vs " << cand;
   
      //skip a rootless subtree
      if (!rootless && lexsmaller(subtree, cand)){
         //cout << " -- SMALLER ";
         res = FK.find(iter-1, subtree, hval);  
         //cout << ((res)? " ** FOUND\n":" ** NOTFOUND\n");
         if (!res) return NULL; //subtree not found!
      }
      //else cout << " -- GREATER " << endl;
   }
   
  
   if (res) eqn = new Eqnode(val,pos);
   else eqn = NULL;

   return eqn;
}


void enumerate_freq(Eqclass *eq, int iter)
{
   TimeTracker tt;
   Eqclass *neq;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *ins, *outs;

   if (prune_type == noprune) eq->sort_nodes(); //doesn't work with pruning

   //cout << "FX " << *eq << endl;

   for (ni = eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));
      tt.Start();
      for (nj = eq->nlist().begin(); nj != eq->nlist().end(); ++nj){ 
         if ((*ni)->pos < (*nj)->pos) continue;

         ins = outs = NULL;
         if ((*ni)->pos > (*nj)->pos){
            outs = test_node(iter, neq, (*nj)->val, (*nj)->pos);
         }
         else{ 
            outs = test_node(iter, neq, (*nj)->val, (*nj)->pos);
            int scnt;
            neq->get_scope(iter-2, scnt);
            ins = test_node(iter, neq, (*nj)->val, scnt);
         }  

         //cout << "prefix " << neq->print_prefix() << " -- " 
         //     << *(*nj) << " " << outs_depth << endl;
         if (ins || outs)
            get_intersect(&(*ni)->tlist, &(*nj)->tlist, ins, outs,(*nj)->pos);
         
         if (outs){
            stats.incrcand(iter-1);
            //cout << "OUTS " << *outs;
            if (notfrequent(*outs)) delete outs;
            else{
               neq->add_node(outs);
               stats.incrlarge(iter-1);
            }
         }
         if (ins){
            // cout << "INS " << *ins;
            stats.incrcand(iter-1);
            if (notfrequent(*ins)) delete ins;
            else{
               neq->add_node(ins);
               stats.incrlarge(iter-1);
            }
         }
      }
      stats.incrtime(iter-1, tt.Stop());
      if (!neq->nlist().empty()){
         if (output){
            //cout << "IIIIII\n";
            cout << "ITER " << iter-1 << endl;
            cout << *neq;
         }
         if (prune_type == prune) FK.add(iter,neq);
         enumerate_freq(neq, iter+1);
      }
      delete neq;
   }
}

void form_f2_lists(Eqclass *eq)
{
   list<Eqnode *>::iterator ni;
   idlist *l1, *l2;
   Eqnode *ins=NULL, *outs=NULL;
   int pit, nit;
   TimeTracker tt;
   
   tt.Start();
   pit = eq->prefix()[0];
   l1 = DCB->Idlists[pit];
   for (ni=eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      nit = (*ni)->val;
      l2 = DCB->Idlists[nit];

      ins = (*ni);
      //cout << "LISTS " << pit << " " << nit << " " << l1->size() 
      //     << " " << l2->size() << " " << ins->tlist.size() << endl;
      get_intersect(l1, l2, ins, outs,-1);
      //cout << "f2prefix " << eq->prefix() << endl;
      //cout << "f2 " << *ins;
   }
   stats.incrtime(1,tt.Stop());
}

void get_Fk(list<Eqclass *> &F2list){
   Eqclass *eq;

   while(!F2list.empty()){
      eq = F2list.front();
      form_f2_lists(eq);
      if (output){
         //cout << "___________\n";         
         cout << *eq;
         //cout << "___________\n";         
      }
      
      if (prune_type == prune) FK.add(2, eq);
      switch(alg_type){
      case treeminer:
         enumerate_freq(eq, 3); 
         break;
      case maxtreeminer:
         cout << "NOT IMPLEMENTED\n";
         break;
      }
      delete eq;
      F2list.pop_front();
   }
}

int main(int argc, char **argv)
{
   TimeTracker tt;
   tt.Start(); 
   parse_args(argc, argv); 
  
   DCB = new Dbase_Ctrl_Blk(infile); 
   get_F1();
   list<Eqclass *> *F2list = get_F2();

   if (output_idlist) DCB->print_vertical();
   get_Fk(*F2list);

   for (int i=0; i < stats.size(); i++){
      cout << "F" << i+1 << " - ";
      cout << stats[i].numlarge << " " << stats[i].numcand << endl;
   }
   
   double tottime = tt.Stop();
   stats.tottime = tottime;

   cout << stats << endl;
   
   cout << "TIME = " << tottime << endl;

   //write results to summary file
   ofstream summary("summary.out", ios::app); 
   summary << "VTREEMINERD ";
   switch(sort_type){
   case incr: summary << "INCR "; break;
   case decr: summary << "DECR "; break;
   default: break;
   }
   switch(prune_type){
   case prune: summary << "PRUNE "; break;
   case itemprune: summary << "ITEMPRUNE-" << prune_value << "-" 
                           << prune_cnt << " ";
   deafult: break;
   }
   if (!count_unique) summary << "MULTIPLE ";
   if (use_fullpath) summary << "FULLPATH ";

   summary << infile << " " << MINSUP_PER << " "
           << DBASE_NUM_TRANS << " " << MINSUPPORT << " ";
   summary << stats << endl;
   summary.close();
   
   exit(0);
}



