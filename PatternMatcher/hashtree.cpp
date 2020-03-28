#include <algorithm>

#include "hashtree.h"
#include "eqclass.h"
#include "treeminer.h"

int HashTree::theCount=0;
int HashTree::theThreshold=DEF_THRESHOLD;
int HashTree::theMaxDepth=0;
 

HashTree::HashTree (int depth)
{
   theDepth = depth;
   theFlg=-1;
   theLeaf = true;
   theEList = NULL;
   theHTable = NULL;
}

HashTree::~HashTree()
{
   if (isleaf()){
      if (theEList){
         for_each(theEList->begin(), theEList->end(), delnode<Eqclass *>());
         delete theEList;
      }
   }
   else{
      HTable::iterator hi = theHTable->begin();
      for(; hi != theHTable->end(); hi++){
         delete (*hi).second;
      }
      delete theHTable;
   }
}

HashTree * HashTree::hash(int Value)
{
   HTable::iterator hi = theHTable->find(Value);
   if (hi == theHTable->end()) return NULL;
   else return (*hi).second;
}

void HashTree::rehash()
{
   //cout<<"*** rehash ***\n";
   //cout<<"theDepth = "<<theDepth<<endl;
   
   int i, val;
   HashTree * hval;
   theHTable = new HTable;
   
   while(!theEList->empty())
   {
      Eqclass *temp = theEList->front();
      
      val = temp->item(theDepth);
      hval = hash(val); // according to current theDepth
      //cout<<"item and hval "<< temp->item(theDepth)<<" "<<hval<<endl;
      if (hval == NULL){
         hval = new HashTree(theDepth+1);
         (*theHTable)[val] = hval;
      }
      
      hval->add_element(temp);
      theEList->pop_front();
      
   }
   
   delete theEList;
   theEList = NULL;
   theLeaf = false;
}



void HashTree::add_element(Eqclass *hel) {
  //cout << "add " << theDepth << " -" << hel->prefix() << endl;
   if (isleaf()) {
      if (theEList == NULL){
         theEList = new list<Eqclass *>;
      }
      
      theEList->push_back(hel);
      
      if (theEList->size() > theThreshold)
         if (theDepth < theMaxDepth)
            rehash();
   }
   else {
      int val = hel->item(theDepth);
      HashTree * hval = hash(val);
      //cout<<"hval = "<<hval<<endl;
      if (hval==NULL){
         hval = new HashTree(theDepth+1);
         (*theHTable)[val] = hval;
      }
      
      hval->add_element(hel);      
   }
}                             

ostream & operator<<(ostream& fout, HashTree& ht){
  int i,j;

  //fout << "in PRINT hashtree\n";
  if (ht.isleaf()){
    list<Eqclass *> *eql = ht.eqlist();
    Eqclass *eq;
    list<Eqclass *>::iterator ni;
    
    for (ni = eql->begin(); ni != eql->end(); ni++){
      for (i=0; i < ht.theDepth; i++) fout << "\t";
      fout << (**ni) << endl;
      //fout << (*ni)->prefix() << " -- ";
      //list<Eqnode *>::iterator nj = (*ni)->nlist().begin();
      //for (; nj != (*ni)->nlist().end(); nj++){
      //	fout << "( " << *nj << " ) ";
      //}
      //fout << endl;
    }    
  }
  else{
     HTable::iterator hi = ht.theHTable->begin();
    for (; hi != ht.theHTable->end(); hi++){
       for (j=0; j < ht.theDepth; j++) fout << "\t";
       fout << "item " << i << endl;
       fout << (*hi).second;
    }
  }
  
  return fout;
}

///////////////////
//FreqHT
///////////////////

void FreqHT::add(Eqclass *eq){
   vector<int> *iset;
   int phval = 0;
   int i;
   for (i=0; i < eq->prefix().size(); i++)
      if (eq->prefix()[i] != BranchIt) phval += eq->prefix()[i];
   
   
   int hval = 0;
   int scope, scnt;
   list<Eqnode>::iterator ni = eq->nlist().begin();
   for (; ni != eq->nlist().end(); ni++){
      iset = new vector<int>(eq->prefix());
      scnt = eq->get_scope((*ni).pos, scope); //what is the scope of node.pos
      while(scnt > scope){
         iset->push_back(BranchIt);
         scnt--;
      }
      iset->push_back((*ni).val);
      hval = phval + (*ni).val;
      int hres = hash_funct()(hval);
      insert(cHTPair(hres, iset));
      //cout << "ADD " << hres << " xx " << *iset << endl;
   }
}

bool eqcmp(vector<int> *v1, vector<int> *v2)
{
   if (v1->size() != v2->size()) return false;
   for (int i=0; i < v1->size(); i++){
      if ((*v1)[i] != (*v2)[i]) return false;
   }
   return true;
}

bool FreqHT::find(vector<int> &cand, int hval)
{
   int hres = hash_funct()(hval);     
   cHTFind p = equal_range(hres);
   
   //cout << "FIND " << hres << " xx  " << cand << endl;
   cHTable::iterator hi = p.first;
   for (; hi!=p.second; hi++){
      if (eqcmp(&cand, (*hi).second)) return true;
   }
   return false;
}
