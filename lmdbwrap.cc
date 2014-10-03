#include "lmdbwrap.hh"
#include <lmdb.h>
#include <iostream>
#include <stdexcept>

using namespace std;

LMDBTransaction::LMDBTransaction(MDB_env* env, LMDBFile::Mode mode)
  : d_env(env)
{
  d_transactionOpen=false;
  int rc;
  if( (rc = mdb_txn_begin(d_env, NULL, mode == LMDBFile::Mode::ReadOnly ? MDB_RDONLY : 0, &d_txn) ))
    throw runtime_error("Couldn't start LMDB txn mdb_txn_begin() returned " + string(mdb_strerror(rc)));

  if( (rc = mdb_dbi_open(d_txn, 0, 0*MDB_DUPSORT, &d_db) ) )
    throw runtime_error("Couldn't open LMDB zone database  mdb_dbi_open() returned " + string(mdb_strerror(rc)));

  d_transactionOpen=true;

}

LMDBTransaction::~LMDBTransaction()
{
  
  // this should close any open transactions, if they were read-only, also their cursors
  if(d_transactionOpen)
    abort();

}

LMDBFile::LMDBFile(const char* fname, LMDBFile::Mode mode)
{
  int rc;
 
  if( (rc = mdb_env_create(&d_env))  )
    throw runtime_error("Couldn't open LMDB database: mdb_env_create() returned " + string(mdb_strerror(rc)));

  if( (rc=mdb_env_set_mapsize(d_env, 1000000000)))
    throw runtime_error("Couldn't open LMDB database: mdb_env_set_mapsize() returned " + string(mdb_strerror(rc)));
  
  if( (rc = mdb_env_open(d_env, fname, (mode == Mode::ReadOnly ? MDB_RDONLY : 0), 0660)) )
    throw runtime_error("Couldn't open LMDB database " +string(fname) + ": mdb_env_open() returned " + mdb_strerror(rc));
}

LMDBFile::~LMDBFile()
{
  //  mdb_dbi_close() // not needed say the docs

  mdb_env_close(d_env);

}

std::unique_ptr<LMDBTransaction> LMDBFile::openTransaction(LMDBFile::Mode mode)
{
  return unique_ptr<LMDBTransaction>(new LMDBTransaction(d_env, mode));
}

void LMDBTransaction::commit()
{
  int rc;
  if( (rc = mdb_txn_commit(d_txn)))
    throw runtime_error("Couldn't commit LMDB txn mdb_txn_commit() returned " + string(mdb_strerror(rc)));
  d_transactionOpen=false;
}

void LMDBTransaction::abort()
{
  mdb_txn_abort(d_txn);
  d_transactionOpen=false;
}


void LMDBTransaction::renew()
{
  int rc;
  mdb_txn_reset(d_txn);

  if( (rc = mdb_txn_renew(d_txn)))
    throw runtime_error("Couldn't reset LMDB txn mdb_txn_renew() returned " + string(mdb_strerror(rc)));

}

void LMDBTransaction::put(const std::string& key, const std::string& value)
{
  put(key.c_str(), key.size(), value.c_str(), value.size());
}

void LMDBTransaction::put(const char* key, unsigned int keysize, const char* value, unsigned int valuesize)
{
  MDB_val k, v;
  k.mv_size = keysize;   k.mv_data = (void*)key;
  v.mv_size = valuesize; v.mv_data = (void*)value;
  int rc;
  if((rc=mdb_put(d_txn, d_db, &k, &v, 0))) 
    throw runtime_error("Couldn't store LMDB data: "+string(mdb_strerror(rc)));
}

bool LMDBTransaction::get(const std::string& key, std::string& value)
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

