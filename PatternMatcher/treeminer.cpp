#include<iostream>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stack>
#include <list>

//headers
#include "treeminer.h"
#include "timetrack.h"
#include "calcdb.h"
#include "eqclass.h"
#include "hashtree.h"
#include "stats.h"

typedef vector<bool> bit_vector;

//global vars
string *infile;
HashTree *CandK = NULL;
FreqHT FK;
Dbase_Ctrl_Blk *DCB;
Stats stats;

double MINSUP_PER;
int MINSUPPORT=-1;
int DBASE_MAXITEM;
int DBASE_NUM_TRANS;

//default flags
bool output = false; //don't print freq subtrees
bool count_unique = true; //count support only once per tree
sort_vals sort_type = nosort; //default is to sort in increasing order
prune_vals prune_type = prune; //prune candidates by default

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
     while ((c=getopt(argc,argv,"bh:i:op:s:S:uz:"))!=-1){
       switch(c){
       case 'b':
          Dbase_Ctrl_Blk::binary_input= true;
          break;
       case 'h': //hash threshold
	 HashTree::threshold() = atoi(optarg);
	 break;         
       case 'i': //input files
	 infile = new string (optarg);
	 //sprintf(infile,"%s",optarg);
	 break;
       case 'o': //print freq subtrees
	 output = true;
	 break;
       case 'p':
          prune_type = (prune_vals) atoi(optarg);
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
  vector<int> itcnt;
  vector<int> flgs;

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
         if (it != BranchIt){
            if (it >= DBASE_MAXITEM){
	      for (j=DBASE_MAXITEM; j <= it; j++){
                  itcnt.push_back(0);
		  flgs.push_back(-1);
	      }
	      DBASE_MAXITEM = it+1;
	      //cout << "IT " << DBASE_MAXITEM << endl;
            }
            
	    if (count_unique){
	      if(flgs[it] == DCB->Cid) continue;
	      else flgs[it] = DCB->Cid;
	    }
            itcnt[it]++;
	    
         }
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
	if (output) cout << i << " - " << itcnt[it_order[i]] << endl;
         DCB->FreqIdx[j] = it_order[i];
         DCB->FreqMap[it_order[i]] = j;
         j++;
      }
      else DCB->FreqMap[it_order[i]] = -1;
   }
   
   cout<< "F1 - " << DCB->NumF1 << " " << DBASE_MAXITEM << endl;  

   if (sort_type != nosort){
      ITCNT = NULL;
      delete [] it_order;
   }

   te = tt.Stop();
   stats.add(DBASE_MAXITEM, DCB->NumF1, te);

}

void get_F2()
{
  int i,j;
  int it1, it2;
  int scnt;
  
  TimeTracker tt;
  double te;

  tt.Start();
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
    
   while(DCB->get_next_trans())
   {
      DCB->get_valid_trans();

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

   CandK = new HashTree(0);
   CandK->maxdepth() = 1;
   if (prune_type == prune) FK.clearall();
   // count frequent patterns and generate eqclass
   Eqclass *eq;
   for (i=0; i < DCB->NumF1; i++) {
      eq = NULL;
      for (j=0; j < DCB->NumF1; j++) {
	//cout << "access " << i << " " << j << endl;
         if (itcnt2[i][j] >= MINSUPPORT){
            F2cnt++;
            if (eq == NULL){
               eq = new Eqclass();
               eq->prefix().push_back(i);
            }
	    eq->add_node(j,0,itcnt2[i][j]);
	    if (output) 
	      cout << DCB->FreqIdx[i] << " " << DCB->FreqIdx[j] 
		   << " - " << itcnt2[i][j] << endl;
         }
      }   
      if (eq != NULL){
         if (prune_type == prune) FK.add(eq);
         CandK->add_element(eq);
         CandK->count()++;
      }
   }
   
   for (i=0; i < DCB->NumF1; i++) {
      //cout << "dealloc " << i << " " << itcnt2[i] << endl;
      delete [] itcnt2[i];
      //cout << "dealloc " << i << " " << flgs[i] << endl;
      delete [] flgs[i];
   }
   
   delete [] itcnt2;
   delete [] flgs;
   
   
   cout << "F2 - " << F2cnt << " " << DCB->NumF1 * DCB->NumF1 << endl;
   te = tt.Stop();
   stats.add(DCB->NumF1 * DCB->NumF1, F2cnt, te);
}


void add_node(int iter, Eqclass *neq, int val, int pos)
{
   if (prune_type == noprune){
      //don't do any pruning
      neq->add_node(val,pos);
      return;
   }
   
   //prune based on frequent subtree
   static vector<int> cand;
   static vector<int> subtree;

   int hval;
   int scope, scnt;

   //form the candidate preifx
   cand = neq->prefix();
   scnt = neq->get_scope(pos, scope); //what is the scope of node.pos
   
   while(scnt > scope){
      cand.push_back(BranchIt);
      scnt--;
   }
   cand.push_back(val); 

   //cout << "CAND " << val << " - " << pos << " -- " << cand << endl;
   
   //check subtrees
   int omita, omitb;
   bool res = true;
   //omit i-th item (omita) and associated BranchIt (omitb)
   int i,j,k;

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
      bool rootless = false;
      scnt = 0; 
      hval = 0;
      subtree.clear();
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
      
      
      if (!rootless){
         res = FK.find(subtree, hval);
         //cout << ((res)? " FOUND\n":"NOTFOUND\n"); 
         if (!res) return; //subtree not found!
      }
   }

   //all subtrees frequent were found, add to cand class
   if (res) neq->add_node(val,pos);
}

void cand_gen(int iter, Eqclass &eq, list<Eqclass *> &neweql)
{
   Eqclass *neq;
   list<Eqnode>::iterator ni, nj;

   //cout << "CAND GEN " << eq << endl;
   
   for (ni = eq.nlist().begin(); ni != eq.nlist().end(); ++ni){
      neq = NULL;
      for (nj = eq.nlist().begin(); nj != eq.nlist().end(); ++nj){
         //cout << "NINJ " << *ni << " -- " << *nj << endl;
         if (ni->pos < nj->pos) continue;
         if (neq == NULL){
            neq = new Eqclass;
            neq->set_prefix(eq.prefix(),*ni);
         }
         if (ni->pos > nj->pos) add_node(iter, neq, nj->val, nj->pos);
         else{ //(ni->pos == nj->pos){
            //if (ni->val <= nj->val) 
            add_node(iter, neq, nj->val, nj->pos);
            add_node(iter, neq, nj->val, neq->prefix().size()-1);
         }
      }
      if (!neq->nlist().empty()){
         neweql.push_back(neq);
         //cout << "NEQCLAS " << *neq << endl;
      }
      else delete neq;
   }
}

void candidate_generation(int iter, HashTree *ht, int &candcnt)
{
   int i;
   
   if (ht->isleaf()){
      list<Eqclass *> *oldeql = ht->eqlist();
      list<Eqclass *> *neweql = new list<Eqclass *>;
      Eqclass *eq;
  
      ht->flag() = -1; //reset the flag
      while(!oldeql->empty()){
         eq = oldeql->front();
	 
	 //cout << "OLD " << *eq << endl;
         cand_gen(iter, *eq, *neweql);
         delete eq;
	 ht->count()--;
         oldeql->pop_front();
      }

      list<Eqclass *>::iterator ni;
      for (ni = neweql->begin(); ni != neweql->end(); ni++){
         ht->add_element(*ni);
	 ht->count()++;
	 candcnt += (*ni)->nlist().size();
      }
      
      delete neweql;
      
   }
   else{
      HTable::iterator hi = ht->htable().begin();
      for (; hi != ht->htable().end(); hi++)
         candidate_generation(iter, (*hi).second, candcnt);
   }
}




ostream & operator<<(ostream& fout, vector<int> &vec){ 
  fout << vec[0];
  for (int i=1; i < vec.size(); i++)
    fout << " " << vec[i];
  return fout;
}


bool incr_nodes (Eqclass *eq, int tpos, int tscope, stack<int> &stk,
		 bit_vector &cflgs)
{
  int i, f, st, en, l;
  bool retval = false;
  int fcnt = 0;
  int scope, ttscope, ttpos;
  stack<int> tstk;
  
  list<Eqnode>::iterator ni = eq->nlist().begin();
  for (f=0; ni != eq->nlist().end(); ni++, f++){

    //if unique counts and node has been counted, skip to next node
    if (count_unique && cflgs[f]){
      fcnt++;
      continue; 
    }

    //for (int d = 0; d < stk.size(); d++) cout << "\t";
    //cout << "search " << ni->val << " " << ni->pos;

    ttscope = tscope;
    scope = ttscope;
    ttpos = tpos;
    bool skip = false;
    int st, en;
    en = eq->get_scope(ni->pos, st);
    if (en > st){
      skip = true;
      while(en > st){
         st++;
         tstk.push(stk.top());
         stk.pop();
      }
      ttscope = tstk.top();
    }
    
    while (skip && scope >= ttscope && ttpos < DCB->TransSz){
      if (DCB->TransAry[ttpos] == BranchIt) scope--;
      else scope++;
      ttpos++;   
    }

    if (skip) ttscope = stk.top();
    //search for the last item within cur_scope
    for (i=ttpos; i < DCB->TransSz; i++){
      if (DCB->TransAry[i] == BranchIt) scope--;
      else scope++;
      
      if (scope < ttscope) break;
      
      if (ni->val == DCB->TransAry[i]){
         //cout << " found at " << i << " " << scope;
         if (count_unique){
            if (!cflgs[f]){
               cflgs[f] = true;
               fcnt++;
               ni->sup++;
            }
         }
         else ni->sup++;
      }
    }
    //cout << endl;

    while (!tstk.empty()){
      stk.push(tstk.top());
      tstk.pop();
    }
  }  
  
  //all nodes have been seen
  if (count_unique && fcnt == cflgs.size()) retval = true;
  
  return retval;  
}

bool incr_support(Eqclass *eq, int tpos, int ppos, int tscope, 
		  stack<int> &stk, bit_vector &cflgs)
{
  int i;
  int scope, ttscope;
  stack<int> tstk;


  scope = tscope;
  bool skip = false;
  if (eq->prefix()[ppos] == BranchIt){
    skip = true;
    while(eq->prefix()[ppos] == BranchIt){
      tstk.push(stk.top());
      stk.pop();
      ppos++;
    }
    tscope = tstk.top();
  }

  while (skip && scope >= tscope && tpos < DCB->TransSz){
    if (DCB->TransAry[tpos] == BranchIt) scope--;
    else scope++;
    tpos++;   
  }
  
  if (skip) tscope = stk.top();

  bool allfound = false;

  for (i=tpos; i < DCB->TransSz && !allfound; i++){
    if (DCB->TransAry[i] == BranchIt) scope--;
    else scope++;
    if (scope < tscope) break;
    if (DCB->TransAry[i] == eq->prefix()[ppos]){
      stk.push(scope);

      //for (int d = 0; d < stk.size(); d++) cout << "\t";

      if (ppos == eq->prefix().size()-1){
         //cout << ppos << " found at " << i << " " << scope << endl;
	allfound = incr_nodes(eq, i+1, scope, stk, cflgs);
      }
      else{
         //cout << ppos << " recurse at " << i << " " << scope << endl;
	allfound = incr_support(eq, i+1, ppos+1, scope, stk, cflgs);
      }
      stk.pop();
    }
    
  }

  while(!tstk.empty()){
    stk.push(tstk.top());
    tstk.pop();
  }
  return allfound;
}

void count_support(HashTree *ht, int st, int en)
{
  static bit_vector cflgs;
  HashTree *hval;
  list<Eqclass *>::iterator ni;
  int i, j, val;

  //if leaf increment candidate counts
  if (ht->isleaf()){
     
     //for (ni = ht->eqlist()->begin(); ni != ht->eqlist()->end(); ni++){
     //   for (i=0; i < ht->depth(); i++) cout << "\t";
     //   cout << "LEAF " << ht->flag() << ": ";
     //   cout << (**ni) << endl;
     //}
     
    //make sure node was not visited before
    if (ht->flag() != DCB->Cid){
      ht->flag() = DCB->Cid;
      for (ni = ht->eqlist()->begin(); ni != ht->eqlist()->end(); ni++){
	
	if (count_unique){
	  for (i=0; i < cflgs.size(); i++) cflgs[i] = false;
	  while (cflgs.size() < (*ni)->nlist().size()){
	    cflgs.push_back(false);
	  }
	}

	//cout << "\nfind " << (*ni)->prefix() << endl;
	//cout << "in trans ";
	//DCB->print_trans();
	

	stack<int> stk;
	incr_support(*ni, 0, 0, 0, stk, cflgs);
      }
    }
  }
  else{ 
    //recurse down the tree
     //for (j=0; j < ht->depth(); j++) cout << "\t";
     //cout << "range " << st << " " << en << endl;
    for (i=st; i < en; i++){

      val = DCB->TransAry[i];
      //for (j=0; j < ht->depth(); j++) cout << "\t";
      //cout << "item " << i << " " << val << endl;

      //skip invalid branching items
      if (val == BranchIt) continue;

      hval = ht->hash(val);
      if (hval) count_support(hval, i+1, en+1);
    }
  }
}

static bool notfrequent (Eqnode &n){
  //cout << "IN FREQ " << n.sup << endl;
  if (n.sup >= MINSUPPORT) return false;
  else return true;
}

bool get_frequent(int iter, HashTree *ht, int &freqcnt)
{
  int i;
  
  bool empty_leaf = false;

  if (ht->isleaf()){
    list<Eqclass *> *eql = ht->eqlist();
    Eqclass *eq;
    list<Eqclass *>::iterator ni;
    
    for (ni = eql->begin(); ni != eql->end() && !eql->empty(); ){
      eq = *ni;
      //cout << "processing " << eq->prefix() << endl;
      list<Eqnode>::iterator nj;
      nj = remove_if(eq->nlist().begin(), eq->nlist().end(), notfrequent);
      eq->nlist().erase(nj, eq->nlist().end());

      freqcnt += eq->nlist().size();
      //cout << "freqcnt  " << freqcnt << " " << eq->nlist().size() << endl;
      if (output && !eq->nlist().empty()) eq->print(DCB);
      
      if (eq->nlist().empty()){
         ni = eql->erase(ni);
         CandK->count()--;
      }
      else{
         //cout << "push to FK " << eq->nlist().size() << endl;
         if (prune_type == prune) FK.add(eq);
         ni++;
      }
    }    
    if (eql->empty()) empty_leaf = true;
  }
  else{
     HTable::iterator ti, hi = ht->htable().begin();
     int ecnt=0;
     for (; hi != ht->htable().end();){
	bool ret = get_frequent(iter, (*hi).second, freqcnt);
	if (ret){
           ecnt++;
           ti = hi;
           hi++;
           ht->htable().erase(ti);
        }    
        else hi++;
     }
     //if (ecnt == ht->htable().size()){
        //delete ht;
     //   empty_leaf = true;
     //}
  }

  return empty_leaf;
}

void get_Fk()
{
  int candcnt, freqcnt;
  TimeTracker tt;
  double te;

  //cout << "FKKK 2 " << FK.size() << endl;
  //for_each(FK.begin(), FK.end(), printout<Eqclass>());
  //cout << "----------------\n";
  
  for (int iter=3; !CandK->isempty(); iter++){
    tt.Start();
    CandK->maxdepth() = iter-1;
    candcnt = 0;
    freqcnt = 0;
    //cout << "FKKK " << iter << ": " << FK.size() << endl;
    //for_each(FK.begin(), FK.end(), printout<Eqclass>());
    //cout << "----------------\n";
    candidate_generation(iter, CandK, candcnt);

    //cout << *CandK << endl;
    if (candcnt > 0){
      while(DCB->get_next_trans()) {
	DCB->get_valid_trans();       
	//DCB->print_trans();
	count_support(CandK, 0, DCB->TransSz-iter+1);
      }
      if (prune_type == prune) FK.clearall();
      get_frequent(iter, CandK, freqcnt);
      //cout << "FKKK " << iter << ": " << FK.size() << endl;
      //for_each(FK.begin(), FK.end(), printout<Eqclass>());
      //cout << "----------------\n";
    }
    cout << "F" << iter << " - " << freqcnt << " " << candcnt << endl;
    
    te = tt.Stop();
    stats.add(candcnt, freqcnt, te);
  }
  if (prune_type == prune) FK.clearall();
}


int main(int argc, char **argv)
{
   TimeTracker tt;
   tt.Start(); 
   parse_args(argc, argv); 
  
   DCB = new Dbase_Ctrl_Blk(infile->c_str()); 
   get_F1();
   get_F2();
   get_Fk();

   double tottime = tt.Stop();
   stats.tottime = tottime;

   cout << stats << endl;
   cout << "TIME = " << tottime << endl;

   //write results to summary file
   ofstream summary("summary.out", ios::app); 
   summary << "HTREEMINER ";
   switch(sort_type){
   case incr: summary << "INCR "; break;
   case decr: summary << "DECR "; break;
   default: break;
   }
   switch(prune_type){
   case prune: summary << "PRUNE "; break;
   deafult: break;
   }
   if (!count_unique) summary << "MULTIPLE ";

   summary << *infile << " " << MINSUP_PER << " "
           << DBASE_NUM_TRANS << " " << MINSUPPORT << " ";
   summary << stats << endl;
   summary.close();
   exit(0);
}

