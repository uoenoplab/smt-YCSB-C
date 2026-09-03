//
//  client.h
//  YCSB-C
//
//  Created by Jinglei Ren on 12/10/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#ifndef YCSB_C_CLIENT_H_
#define YCSB_C_CLIENT_H_

#include <string>
#include <mutex>
#include "db.h"
#include "core_workload.h"
#include "utils.h"

namespace ycsbc {

class Client {
 public:
  Client(DB &db, CoreWorkload &wl) : db_(db), workload_(wl) { }
  
  virtual bool DoInsert();
  virtual bool DoTransaction();
  
  virtual ~Client() { }
  
 protected:
  
  virtual int TransactionRead();
  virtual int TransactionReadModifyWrite();
  virtual int TransactionScan();
  virtual int TransactionUpdate();
  virtual int TransactionInsert();
  
  DB &db_;
  CoreWorkload &workload_;
};

inline bool Client::DoInsert() {
  /* The workload generators are shared across threads: generate under the
   * workload mutex, but keep the (blocking) DB call outside it. */
  std::string key;
  std::vector<DB::KVPair> pairs;
  {
    std::lock_guard<std::mutex> lock(workload_.mutex());
    key = workload_.NextSequenceKey();
    workload_.BuildValues(pairs);
  }
  return (db_.Insert(workload_.NextTable(), key, pairs) == DB::kOK);
}

inline bool Client::DoTransaction() {
  Operation op;
  {
    std::lock_guard<std::mutex> lock(workload_.mutex());
    op = workload_.NextOperation();
  }
  int status = -1;
  switch (op) {
    case READ:
      status = TransactionRead();
      break;
    case UPDATE:
      status = TransactionUpdate();
      break;
    case INSERT:
      status = TransactionInsert();
      break;
    case SCAN:
      status = TransactionScan();
      break;
    case READMODIFYWRITE:
      status = TransactionReadModifyWrite();
      break;
    default:
      throw utils::Exception("Operation request is not recognized!");
  }
  assert(status >= 0);
  return (status == DB::kOK);
}

inline int Client::TransactionRead() {
  const std::string &table = workload_.NextTable();
  std::string key;
  std::vector<std::string> fields;
  {
    std::lock_guard<std::mutex> lock(workload_.mutex());
    key = workload_.NextTransactionKey();
    if (!workload_.read_all_fields())
      fields.push_back("field" + workload_.NextFieldName());
  }
  std::vector<DB::KVPair> result;
  if (!fields.empty())
    return db_.Read(table, key, &fields, result);
  else
    return db_.Read(table, key, NULL, result);
}

inline int Client::TransactionReadModifyWrite() {
  const std::string &table = workload_.NextTable();
  std::string key;
  std::vector<std::string> fields;
  {
    std::lock_guard<std::mutex> lock(workload_.mutex());
    key = workload_.NextTransactionKey();
    if (!workload_.read_all_fields())
      fields.push_back("field" + workload_.NextFieldName());
  }
  std::vector<DB::KVPair> result;
  if (!fields.empty())
    db_.Read(table, key, &fields, result);
  else
    db_.Read(table, key, NULL, result);

  std::vector<DB::KVPair> values;
  {
    std::lock_guard<std::mutex> lock(workload_.mutex());
    if (workload_.write_all_fields())
      workload_.BuildValues(values);
    else
      workload_.BuildUpdate(values);
  }
  return db_.Update(table, key, values);
}

inline int Client::TransactionScan() {
  const std::string &table = workload_.NextTable();
  std::string key;
  int len;
  std::vector<std::string> fields;
  {
    std::lock_guard<std::mutex> lock(workload_.mutex());
    key = workload_.NextTransactionKey();
    len = workload_.NextScanLength();
    if (!workload_.read_all_fields())
      fields.push_back("field" + workload_.NextFieldName());
  }
  std::vector<std::vector<DB::KVPair>> result;
  if (!fields.empty())
    return db_.Scan(table, key, len, &fields, result);
  else
    return db_.Scan(table, key, len, NULL, result);
}

inline int Client::TransactionUpdate() {
  const std::string &table = workload_.NextTable();
  std::string key;
  std::vector<DB::KVPair> values;
  {
    std::lock_guard<std::mutex> lock(workload_.mutex());
    key = workload_.NextTransactionKey();
    if (workload_.write_all_fields())
      workload_.BuildValues(values);
    else
      workload_.BuildUpdate(values);
  }
  return db_.Update(table, key, values);
}

inline int Client::TransactionInsert() {
  const std::string &table = workload_.NextTable();
  std::string key;
  std::vector<DB::KVPair> values;
  {
    std::lock_guard<std::mutex> lock(workload_.mutex());
    key = workload_.NextSequenceKey();
    workload_.BuildValues(values);
  }
  return db_.Insert(table, key, values);
} 

} // ycsbc

#endif // YCSB_C_CLIENT_H_
