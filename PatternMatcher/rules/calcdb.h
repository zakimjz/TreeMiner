#ifndef __DATABASE_H
#define __DATABASE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <hash_map>
#include "countfreq.h"
#include "idlist.h"

using namespace std;

#define HASHNS __gnu_cxx
#define ITSZ sizeof(int)
#define DCBBUFSZ 2048
#define TRANSOFF 3

typedef vector<bool> bit_vector;
typedef HASHNS::hash_map<int, idlist, HASHNS::hash<int>, equal_to<int> > IdLists;
typedef pair<IdLists::iterator, IdLists::iterator> ILfind;
typedef IdLists::value_type ILpair;



class Dbase_Ctrl_Blk{
private:

  //vars related to the horizontal format
   ifstream fd;
   int buf_size;
   int * buf;
   int cur_blk_size; 
   int cur_buf_pos;
   int endpos;
   char readall;   
   
   //vars for the vertical format

public:
   static int NumF1;   //number of freq items

   //vars related to the horizontal format
   static int *TransAry;
   static int TransSz;
   static int Tid;
   static int Cid;
   static int MaxTransSz;
   
   //vars related to vertical format
   IdLists Idlists;
   bit_vector valid_items;
   
   //function definitions
   Dbase_Ctrl_Blk(const char *infile, const int buf_sz=DCBBUFSZ);
   ~Dbase_Ctrl_Blk();
   
   //functions for horizontal format
   void get_next_trans_ext();
   void get_first_blk();
   int get_next_trans();
   void print_trans();
   int eof(){return (readall == 1);}

   //functions for vertical format
   void make_vertical();
   void print_vertical();
};

#endif //__DATABASE_H





