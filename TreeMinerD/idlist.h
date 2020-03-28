#ifndef __idlist_H
#define __idlist_H
 
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

//basic idnode, tidnode, idlist declarations
enum ival_type {equals, sup, sub, before, after, overlap};

//interval class
class ival{
public:
   int lb; //lower bound of scope range
   int ub; //upper bound of scope range
   
   ival(int l=0, int u=0): lb(l), ub(u){};

   static ival_type compare(ival &n1, ival &n2){
      ival_type retval;
      if (n1.lb == n2.lb && n1.ub == n2.ub) retval = equals; //n1 equals n2
      else if (n1.lb < n2.lb && n1.ub >= n2.ub) retval = sup; //n1 contains n2
      else if (n1.lb > n2.lb && n1.ub <= n2.ub) retval = sub; //n2 contains n1
      else if (n1.ub < n2.lb) retval = before; //n1 comes before n2
      else if (n2.ub < n1.lb) retval = after; //n1 comes after n2
      else{ 
         retval = overlap;
         //cout << "wrong case in compare_interval: ";
         //cout << "[" << n1.lb << "," << n1.ub << "]";
         //cout << " [" << n2.lb << "," << n2.ub << "]" << endl;
      }
      return retval;
   };

   bool operator == (ival &idn){
      if (lb == idn.lb && ub == idn.ub) return true;
      else return false;
   }
   
   friend ostream & operator<<(ostream& ostr, ival& idn);
};

#define DEFVAL -1
class idnode{
private:
   ival _scope; //this node's scope
   vector<ival> _pslist; //vector of prefix scopes for rightmost path
   int _cid; //Tree Id
public:
   
   idnode(int c=0, int l=0, int u=0):_cid(c), _scope(l,u){}
   
   idnode(int c, ival &ivl): _cid(c), _scope(ivl){}
   
   idnode(idnode& idn, ival &ivl): 
      _cid(idn._cid), _scope(idn._scope), _pslist(idn._pslist){}

   ival &scope() { return _scope; }
   vector<ival> &prefixlist() { return _pslist; }
   int &cid() {return _cid;}
   
   void add_scope(vector<ival> &rest, ival &last){
      for (int i=0; i < rest.size(); ++i){
         _pslist.push_back(rest[i]);
      }
      _pslist.push_back(last);
   }

   void add_scope(vector<ival> &rest, int pos){
      for (int i=0; i < rest.size() && i <= pos; ++i){
         _pslist.push_back(rest[i]);
      }
   }
   

   //return true and ival at pos ivl, else return false
   bool get_ival(int pos, ival &ivl){
      if (pos >= _pslist.size()) return false;
      else{
         ivl = _pslist[pos];
         return true;
      }
   }
   
   //does idn contain this upto pos 
   bool path_contains(idnode &idn, int pos){
      for (int i = 0; i < pos+1; ++i){
         ival_type cmpval = ival::compare(_pslist[i], idn._pslist[i]);
         if (cmpval != equals && cmpval != sub && cmpval != sup) 
            return false;
         if (cmpval == sup){
            cmpval = ival::compare(_scope, idn._pslist[i]);
            if (cmpval != after) return false;
         }
      }
      return true;
   }

   friend ostream & operator<<(ostream& ostr, idnode& idn);
};

class idlist: public vector<idnode>{
   friend ostream & operator<<(ostream& fout, idlist& idl);
};

extern ostream & operator<<(ostream& fout, vector<int> &vec);
#endif
