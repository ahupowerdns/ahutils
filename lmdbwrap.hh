#pragma once
#include <lmdb.h>
#include <string>
#include <lmdb.h>
#include <memory>
#include <boost/utility.hpp>

class LMDBTransaction;

class LMDBFile
{
public:
  enum class Mode {ReadOnly, ReadWrite};

  LMDBFile(const char* fname, Mode mode=Mode::ReadWrite);
  ~LMDBFile();

  std::unique_ptr<LMDBTransaction> openTransaction(LMDBFile::Mode mode=Mode::ReadWrite);
  
private:
  MDB_env* d_env;
};


 
class LMDBTransaction : public boost::noncopyable
{
public:
  LMDBTransaction(MDB_env* env, LMDBFile::Mode mode);

  ~LMDBTransaction();
  void put(const std::string& key, const std::string& value);
  void put(const char* key, unsigned int keysize, const char* value, unsigned int valuesize);

  bool get(const std::string& key, std::string& value);
  void commit();
  void abort();
  void renew();

private:
  MDB_env* d_env;
  MDB_txn* d_txn;
  MDB_dbi d_db;
  bool d_transactionOpen;

};

