#include <iostream>
#include "calcdb.h"
#include "countfreq.h"

using namespace std;
int Dbase_Ctrl_Blk::MaxTransSz=0;
int Dbase_Ctrl_Blk::TransSz=0;
int *Dbase_Ctrl_Blk::TransAry=NULL;
int Dbase_Ctrl_Blk::Tid=0;
int Dbase_Ctrl_Blk::Cid=0;
int Dbase_Ctrl_Blk::NumF1=0;

Dbase_Ctrl_Blk::Dbase_Ctrl_Blk(const char *infile, const int buf_sz)
{
   fd.open(infile, ios::in|ios::binary);
   if (!fd){
      cerr << "cannot open infile" << infile << endl;
      exit(1);
   }

   buf_size = buf_sz;
   buf = new int [buf_sz];
   cur_buf_pos = 0;
   cur_blk_size = 0;
   readall = 0;
   fd.seekg(0,ios::end);
   endpos = fd.tellg();
   fd.seekg(0,ios::beg);
}
   
Dbase_Ctrl_Blk::~Dbase_Ctrl_Blk()
{
   delete [] buf;
   fd.close();
}

void Dbase_Ctrl_Blk::get_first_blk()
{
   readall=0;

   fd.clear();
   fd.seekg(0,ios::beg);
   
   fd.read((char *)buf, (buf_size*ITSZ));
   cur_blk_size = fd.gcount()/ITSZ; 
   if (cur_blk_size < 0){
      cerr << "problem in get_first_blk" << cur_blk_size << endl;
   }
   
   cur_buf_pos = 0;
}

int Dbase_Ctrl_Blk::get_next_trans ()
{
   static char first=1;  

   if (first){
      first = 0;
      get_first_blk();
   }

   if (cur_buf_pos+TRANSOFF >= cur_blk_size ||
       cur_buf_pos+buf[cur_buf_pos+TRANSOFF-1]+TRANSOFF > cur_blk_size){
      fd.seekg(0,ios::cur);
      if (fd.tellg() == endpos) readall = 1;      
      if (!readall){
         // Need to get more items from file
         get_next_trans_ext();
      }      
   }
   
   if (eof()){
      first = 1;
      return 0;
   }                     
   
   if (!readall){
      Cid = buf[cur_buf_pos];
      Tid = buf[cur_buf_pos+TRANSOFF-2];
      TransSz = buf[cur_buf_pos+TRANSOFF-1];
      TransAry = buf + cur_buf_pos + TRANSOFF;
      cur_buf_pos += TransSz + TRANSOFF;
   }
   
   return 1;
}

void Dbase_Ctrl_Blk::get_next_trans_ext()
{
   // Need to get more items from file
   int res = cur_blk_size - cur_buf_pos;
   if (res > 0)
   {
      // First copy partial transaction to beginning of buffer
      for (int i=0; i < res; i++)
         buf[i] = buf[cur_buf_pos+i]; 
      cur_blk_size = res;
   }
   else
   {
      // No partial transaction in buffer
      cur_blk_size = 0;
   }

   fd.read((char *)(buf + cur_blk_size),
           ((buf_size - cur_blk_size)*ITSZ));
   
   res = fd.gcount();
   if (res < 0){
      cerr << "in get_next_trans_ext" << res << endl;
   }
   
   cur_blk_size += res/ITSZ;
   cur_buf_pos = 0;
}


void Dbase_Ctrl_Blk::print_trans(){
  cout << Cid << " " << Tid << " " << TransSz;
  for (int i=0; i < TransSz; i++)
    cout << " " << TransAry[i];
  cout << endl;
}


void Dbase_Ctrl_Blk::make_vertical(){
   int i, j; //track the position in trans, counting BranchIt
   int pi, pj; //track the position in trans, not counting BranchIt
   int scope;
   
  //convert current transaction into vertical format
  for (i=0, pi=0; i < TransSz; i++){
    if (TransAry[i] == BranchIt) continue;

    scope=0;
    for (j=i+1, pj=pi; scope >= 0 && j < TransSz; j++){
      if (TransAry[j] == BranchIt) scope--;
      else{
         scope++;
         pj++;
      }
    }
    
    if (TransAry[i] >= valid_items.size()){
          valid_items.resize(TransAry[i], false);
    }
    valid_items[TransAry[i]] = true;
    
    idnode nn(Cid, pi,pj);
    //cout << "push " << TransAry[i] << " " <<  pi << " " << pj << endl;
    Idlists[TransAry[i]].push_back(nn);
    pi++;
  }
}

void Dbase_Ctrl_Blk::print_vertical(){
   int i;
   cout << "VI " << valid_items.size() << endl;
   for (i=0; i < valid_items.size(); ++i){
      if (valid_items[i]){
         cout << "item " << i << endl;
         cout << Idlists[i];
      }
      
   }
}
