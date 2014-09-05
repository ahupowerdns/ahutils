#pragma once
#include <lmdb.h>
#include <string>

class LMDBFile
{
public:
  enum class Mode {ReadOnly, ReadWrite};

  LMDBFile(const char* fname, Mode mode=Mode::ReadWrite);
  ~LMDBFile();
  void openTransaction(LMDBFile::Mode mode=Mode::ReadWrite);
  void commitTransaction();
  void abortTransaction();
  void renewTransaction();
  void put(const std::string& key, const std::string& value);
  void put(const char* key, unsigned int keysize, const char* value, unsigned int valuesize);

  bool get(const std::string& key, std::string& value);
private:
  MDB_env* d_env;
  MDB_txn* d_txn;
  MDB_dbi d_db;
  bool d_transactionOpen;
};

