#include<iostream>
#include <fstream>
#include <strstream>
#include <unistd.h>
#include <algorithm>
#include <stdio.h>
//#include <stl.h>
#include <list>
#include <stack>
#include <cstring>


//headers
#include "countfreq.h"
#include "timetrack.h"
#include "calcdb.h"
#include "eqclass.h"
#include "freqit.h"

using namespace std;

//global vars
char ffile[300];
char dbfile[300];
Dbase_Ctrl_Blk *DCB;

bool count_unique;

int SUPPORT;
int DBASE_MAXITEM;
int DBASE_NTRANS;
int F_DBASE_NTRANS;
int F_MINSUPPORT;

FreqAry fary;

void parse_args(int argc, char **argv)
{
   extern char * optarg;
   int c;
   
   if (argc < 2)
      cout << "usage: treeminer -i<infile> -s <support>\n";
   else{
      while ((c=getopt(argc,argv,"f:d:"))!=-1){
         switch(c){
         case 'f': //freq pattern file
            sprintf(ffile,"%s",optarg);
            break;
         case 'd': //db file in which to count
            sprintf(dbfile,"%s",optarg);
            break;
         }               
      }
   }
}


void read_freq(const char *ffile)
{
   const int lineSize=1024;
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
   while(!skipfile && fin.getline(inBuf, lineSize)){
      inSize = fin.gcount();
      int it;
      istrstream ist(inBuf, inSize);
      fit = NULL;
      while(ist >> inStr){
         if (strcmp (inStr, "DBASE_NUM_TRANS") == 0){
            ist >> inStr; //skip :
            ist >> inStr;
            F_DBASE_NTRANS = atoi(inStr);
            break;
         }
         else if (strcmp (inStr, "DBASE_MAXITEM") == 0){
            break; // skip line
         }
         else if (strcmp (inStr, "MINSUPPORT") == 0){
            ist >> inStr; //skip :
            ist >> inStr;
            F_MINSUPPORT = atoi(inStr);
            break;
         }
         else if (inStr[0] == 'F'){ 
            //cout << "came here " << inStr << endl;
            break; //skip line
         }
         else if (strcmp (inStr, "[") == 0){
            //cout << "came skip here " << inStr << endl;
            skipfile = true; //skip rest of file
            break; 
         }
         else{
            if (!fit) fit = new FreqIt;
            //process the frequent tree
            if (strcmp(inStr, "-") == 0){
               ist >> inStr; //skip -
               int sup = atoi(inStr);
               fit->fsup = sup;
               fit->fratio = ((double) sup / F_DBASE_NTRANS);
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


void incr_support(FreqIt &fit, int tpos, int ppos, int tscope,
                  stack<int> &stk)
{
   static bool foundflg = false;
   int i;
   int scope, ttscope;
   stack<int> tstk;

   if (tpos == 0) foundflg = false; //init each time new trans is called
   
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
   
   while (skip && scope >= tscope && tpos < DCB->TransSz){
      if (DCB->TransAry[tpos] == BranchIt) scope--;
      else scope++;
      tpos++;
   }
   
   if (skip) tscope = stk.top();
   
   for (i=tpos; i < DCB->TransSz && !foundflg; i++){
      if (DCB->TransAry[i] == BranchIt) scope--;
      else scope++;
      if (scope < tscope) break;
      if (DCB->TransAry[i] == fit[ppos]){
         stk.push(scope);
         
         //for (int d = 0; d < stk.size(); d++) cout << "\t";
         
         if (ppos == fit.size()-1){
            //cout << ppos << " found at " << i << " " << scope << endl;
            //allfound = incr_nodes(fit, i+1, scope, stk);
            fit.dbsup++;
            foundflg = true;
         }
         else{
            //cout << ppos << " recurse at " << i << " " << scope << endl;
            incr_support(fit, i+1, ppos+1, scope, stk);
         }
         stk.pop();
      }
      
   }
   
   while(!tstk.empty()){
      stk.push(tstk.top());
      tstk.pop();
   }
}


void count_support()
{
   int i;
   for (i=0; i < fary.size(); ++i){
      stack<int> stk;
      incr_support(*fary[i], 0, 0, 0, stk);
   }
}

int main(int argc, char **argv)
{
   TimeTracker tt;
   tt.Start(); 
   parse_args(argc, argv); 
  
   DCB = new Dbase_Ctrl_Blk(dbfile); 

   read_freq (ffile);

   DBASE_NTRANS = 0;
   while(DCB->get_next_trans())
   {
      //DCB->print_trans();
      count_support();
      DBASE_NTRANS++;
   }
   
   for (int i=0; i < fary.size(); ++i){
      fary[i]->dbratio = ((double)fary[i]->dbsup) / DBASE_NTRANS;
      cout << *fary[i] << endl;
   }

   double tottime = tt.Stop();

   cout << "TIME = " << tottime << endl;

   exit(0);
}



