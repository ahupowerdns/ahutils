#include "lmdbwrap.hh"
#include <lmdb.h>
#include <iostream>
#include <stdexcept>

using namespace std;

LMDBFile::LMDBFile(const char* fname, LMDBFile::Mode mode)
{
  int rc;
  d_transactionOpen=false;
  if( (rc = mdb_env_create(&d_env))  )
    throw runtime_error("Couldn't open LMDB database: mdb_env_create() returned " + string(mdb_strerror(rc)));

  if( (rc=mdb_env_set_mapsize(d_env, 1000000000)))
    throw runtime_error("Couldn't open LMDB database: mdb_env_set_mapsize() returned " + string(mdb_strerror(rc)));
  
  if( (rc = mdb_env_open(d_env, fname, (mode == Mode::ReadOnly ? MDB_RDONLY : 0), 0660)) )
    throw runtime_error("Couldn't open LMDB database " +string(fname) + ": mdb_env_open() returned " + mdb_strerror(rc));
}

LMDBFile::~LMDBFile()
{
  // this should close any open transactions, if they were read-only, also their cursors
  if(d_transactionOpen)
    abortTransaction();
  //  mdb_dbi_close() // not needed say the docs

  mdb_env_close(d_env);

}

void LMDBFile::openTransaction(LMDBFile::Mode mode)
{
  int rc;
  if( (rc = mdb_txn_begin(d_env, NULL, mode == Mode::ReadOnly ? MDB_RDONLY : 0, &d_txn) ))
    throw runtime_error("Couldn't start LMDB txn mdb_txn_begin() returned " + string(mdb_strerror(rc)));

  if( (rc = mdb_dbi_open(d_txn, 0, 0, &d_db) ) )
    throw runtime_error("Couldn't open LMDB zone database  mdb_dbi_open() returned " + string(mdb_strerror(rc)));

  d_transactionOpen=true;
}

void LMDBFile::commitTransaction()
{
  int rc;
  if( (rc = mdb_txn_commit(d_txn)))
    throw runtime_error("Couldn't commit LMDB txn mdb_txn_commit() returned " + string(mdb_strerror(rc)));
  d_transactionOpen=false;
}

void LMDBFile::abortTransaction()
{
  mdb_txn_abort(d_txn);
  d_transactionOpen=false;
}


void LMDBFile::renewTransaction()
{
  int rc;
  mdb_txn_reset(d_txn);

  if( (rc = mdb_txn_renew(d_txn)))
    throw runtime_error("Couldn't reset LMDB txn mdb_txn_renew() returned " + string(mdb_strerror(rc)));

}



void LMDBFile::put(const std::string& key, const std::string& value)
{
  put(key.c_str(), key.size(), value.c_str(), value.size());
}

void LMDBFile::put(const char* key, unsigned int keysize, const char* value, unsigned int valuesize)
{
  MDB_val k, v;
  k.mv_size = keysize;   k.mv_data = (void*)key;
  v.mv_size = valuesize; v.mv_data = (void*)value;
  int rc;
  if((rc=mdb_put(d_txn, d_db, &k, &v, 0))) 
    throw runtime_error("Couldn't store LMDB data: "+string(mdb_strerror(rc)));
}

bool LMDBFile::get(const std::string& key, std::string& value)
{
  MDB_val k, v;
  k.mv_size = key.size();   k.mv_data = (void*)key.c_str();

  int rc;
  rc=mdb_get(d_txn, d_db, &k, &v);
  if(rc==MDB_NOTFOUND) {
    value.clear();
    return false;
  }
  else if(rc)
    throw runtime_error("Couldn't retrieve LMDB data: "+string(mdb_strerror(rc)));

  value.assign((const char*)v.mv_data, v.mv_size);
  return true;
}

int main(int argc, char**argv)
try
{
  LMDBFile lf("./test"), lf2("./test");

  lf.openTransaction();
  lf2.openTransaction(LMDBFile::Mode::ReadOnly);
  string value;
  if(lf.get("hallo", value)) {
    cout<<"Found something before: "<<value<<endl;
  }

  cout<<"Storing a key"<<endl;
  lf.put("hallo", "bert");
  if(lf2.get("hallo", value)) {
    cout<<"lf2 also saw the value: "<<value<<endl;
  }
  else 
    cout<<"lf2 did not see it yet"<<endl;
  
  cout<<"Commiting the store"<<endl;
  lf.commitTransaction();
  cout<<"lf committed"<<endl;


  if(lf2.get("hallo", value)) {
    cout<<"lf2 now also saw the value: "<<value<<endl;
  }
  else 
    cout<<"lf2 still does not see it"<<endl;

  cout<<"Renewing the lf2 transaction"<<endl;
  lf2.renewTransaction();
  if(lf2.get("hallo", value)) {
    cout<<"lf2 now also saw the value: "<<value<<endl;
  }
  else 
    cout<<"lf2 still does not see it"<<endl;

  lf2.commitTransaction();
  lf2.openTransaction();
  cout<<"Storing a million values.."<<endl;
  for(unsigned int n=0; n < 1000000; ++n) {
    lf2.put((const char*)&n, 4, (const char*)&n, 4);
  }
  cout<<"Committing"<<endl;
  lf2.commitTransaction();
  cout<<"Done!"<<endl;
  lf.openTransaction(LMDBFile::Mode::ReadOnly);
  string key;
  for(unsigned int n=0; n < 1000000; ++n) {
    key.assign((const char*)&n, 4);
    if(!lf.get(key, value))  {
      cerr<<"Value "<<n<<" was missing!"<<endl;
    }
    if(key!=value)
      cerr<<"Value "<<n<<" was stored incorrectly!"<<endl;
  }
  cout<<"Done finding the entries that should be there"<<endl;
  lf.commitTransaction();

  lf2.openTransaction();
  cout<<"Storing the next million values.."<<endl;
  for(unsigned int n=1000000; n < 2000000; ++n) {
    lf2.put((const char*)&n, 4, (const char*)&n, 4);
  }
  cout<<"Aborting!"<<endl;
  lf2.abortTransaction();

  lf2.openTransaction(LMDBFile::Mode::ReadOnly);
  for(unsigned int n=1000000; n < 2000000; ++n) {
    key.assign((const char*)&n, 4);
    if(lf2.get(key, value))
      cout<<"Found value "<<n<<" anyhow, should not be there!"<<endl;
  }
  cout<<"Verified that all aborted values weren't there"<<endl;


}
catch(exception& e)
{
  cerr<<"Fatal: "<<e.what()<<endl;
}
