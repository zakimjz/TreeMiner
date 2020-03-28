//USAGE: count_pairs <freq_par_file>
// program read the patterns and computes for all pairs if one is a subtree
// of the other or not.

#include <iostream>
#include <fstream>
#include <strstream>
#include <vector>
#include <stack>
#include <cstring>

using namespace std;

#define BranchIt -1 //-1 indicates a branch in the tree

//four possible values of a comparison
enum comp_vals {equals, subset, superset, notequal};

//structure to store frequent subtrees with sup and ratio info for two classes

class FreqIt: public vector<int>{
public:
   FreqIt():sup1(0), sup2(0), ratio1(0.0), ratio2(0.0){}
   void print(){
      int i;
      for (i=0 ; i < size(); ++i){
         cout << (*this)[i] << " ";
      }
      
      cout << "- " << (*this).sup1 << " " << (*this).sup2;
      cout << " " << (*this).ratio1 << " " << (*this).ratio2;
      cout << endl;
   }
   
   int sup1; //frequency in class 1 and 2 DB
   int sup2; 
   double ratio1; //ratio in c1 and c2 DB
   double ratio2;
};

typedef vector<FreqIt *> FreqAry;

FreqAry fary;

bool count_unique=true;


void read_freq(const char *ffile)
{
   const int lineSize=8192;
   const int wdSize=256;

   FreqIt *fit;
   
   char inBuf[lineSize];
   char inStr[wdSize];
   int inSize;
   
   ifstream fin(ffile, ios::in);
   if (!fin){
      cerr << "cannot open freq seq file\n";
      exit(-1);
   }

   bool skipfile = false;
   while(fin.getline(inBuf, lineSize)){
      inSize = fin.gcount();
      int it;
      istrstream ist(inBuf, inSize);
      fit = NULL;
      while(ist >> inStr){
         if (strcmp (inStr, "TIME") == 0){
            break; //skip line
         }
         else{
            if (!fit) fit = new FreqIt;
            //process the frequent tree
            if (strcmp(inStr, "-") == 0){
               ist >> inStr; //read next item
               fit->sup1 = atoi(inStr);
               ist >> inStr; //read next item
               fit->sup2 = atoi(inStr);
               ist >> inStr; //read next item
               fit->ratio1 = atof(inStr);
               ist >> inStr; //read next item
               fit->ratio2 = atof(inStr);
               //cout << "- " << sup << endl;
               fary.push_back(fit);
               break;
            }
            it = atoi(inStr);
            fit->push_back(it);
            //cout << it << " ";
         }
      }
   }
   fin.close();

}


void check_subtree(FreqIt &fit, FreqIt &fit2, 
                  int tpos, int ppos, int tscope,
                  stack<int> &stk, bool &foundflg)
{
   int i;
   int scope, ttscope;
   stack<int> tstk;

   scope = tscope;
   bool skip = false;
   if (fit[ppos] == BranchIt){
      skip = true;
      while(fit[ppos] == BranchIt){
         tstk.push(stk.top());
         stk.pop();
         ppos++;
      }
      tscope = tstk.top();
   }
   
   while (skip && scope >= tscope && tpos < fit2.size()){
      if (fit2[tpos] == BranchIt) scope--;
      else scope++;
      tpos++;
   }
   
   if (skip) tscope = stk.top();
   
   for (i=tpos; i < fit2.size() && !foundflg; i++){
      if (fit2[i] == BranchIt) scope--;
      else scope++;
      if (scope < tscope) break;
      if (fit2[i] == fit[ppos]){
         stk.push(scope);
         
         //for (int d = 0; d < stk.size(); d++) cout << "\t";
         
         if (ppos == fit.size()-1){
            //cout << ppos << " found at " << i << " " << scope << endl;
            //fit.dbsup++;
            foundflg = true;
         }
         else{
            //cout << ppos << " recurse at " << i << " " << scope << endl;
            check_subtree(fit, fit2, i+1, ppos+1, scope, stk, foundflg);
         }
         stk.pop();
      }
      
   }
   
   while(!tstk.empty()){
      stk.push(tstk.top());
      tstk.pop();
   }
}

comp_vals compare(FreqIt &fit1, FreqIt &fit2)
{
   stack<int> stk;
   bool foundflg = false;

   comp_vals res = notequal;
   
   if (fit1.size() <= fit2.size()){      
      check_subtree(fit1, fit2, 0, 0, 0, stk, foundflg);
      //cout << "came here " << fit1.size() << " " << fit2.size() << endl;
      if (foundflg){
         if (fit1.size() == fit2.size()) res = equals;
         else res = subset;
      }
      else res = notequal;
   }
   else{
      check_subtree(fit2, fit1, 0, 0, 0, stk, foundflg);
      if (foundflg) res = superset;
      else res = notequal;
   }
   
   return res;
}


void compare_pairs()
{
   int i,j;
   for (i=0; i < fary.size(); ++i){
      for (j=0; j < fary.size(); ++j){

         //compare pat1 vs pat2
         comp_vals cv = compare (*fary[i], *fary[j]);
         
         cout << "PAT1: ";
         fary[i]->print();
         cout << "PAT2: ";
         fary[j]->print();
         
         cout << "RESULT :";
         switch (cv){
         case equals: cout << "EQUAL"; break;
         case notequal: cout << "NOTEQUAL"; break;
         case subset: cout << "SUBSET"; break;
         case superset: cout << "SUPERSET"; break;
         }
         cout << endl << endl;

      }
   }
}


int main(int argc, char **argv)
{

   //read patterns from pat file
   read_freq (argv[1]);

   //compare all pairs of pats
   compare_pairs();
   
   exit(0);
}



