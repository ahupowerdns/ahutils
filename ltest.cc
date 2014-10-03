#include <stdio.h>
#include <thread>
#include <stdexcept>
#include <sys/types.h>
#include <future>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include "lmdbwrap.hh"

using namespace std;

void unixDie(const char* why)
{
  throw runtime_error("Fatal error during "+string(why)+": "+string(strerror(errno)));
}

vector<int> fillDB(LMDBFile* lf)
{
  auto lt=lf->openTransaction();
  vector<int> ret;
  int r;
  for(int n=0; n < 1000000;++n) {
    r=random();
    lt->put(to_string(r), "hi");
    ret.push_back(r);
  }
  lt->commit();
  return ret;
}


int main(int argc, char**argv)
try
{
  LMDBFile lf("./test");

  auto t1=async(launch::async, fillDB, &lf);
  auto t2=async(launch::async, fillDB, &lf);

  auto res1=t1.get();
  auto res2=t2.get();

  auto lt=lf.openTransaction(LMDBFile::Mode::ReadOnly);
  string value;
  for(auto& r : {res1, res2}) {
    for(auto i : r) {
      if(!lt->get(to_string(i), value))
	cerr<<"missing value "<<i<<endl;
    }
  }
  cout<<"All "<<res1.size()+res2.size()<<" entries were found"<<endl;
}
catch(exception& e)
{
  cerr<<"Fatal: "<<e.what()<<endl;
}
