//
//  redis_db.cc
//  YCSB-C
//

#include "redis_db.h"

#include <cassert>
#include <cstring>

using namespace std;

namespace ycsbc {

namespace {
/* One connection per worker thread; valid between RedisDB::Init() and
 * RedisDB::Close(). There is a single RedisDB instance, so no per-instance
 * key is needed. */
thread_local RedisClient *t_client = nullptr;
}

void RedisDB::Init() {
  assert(t_client == nullptr);
  t_client = new RedisClient(host_.c_str(), port_, slaves_, transport_);
}

void RedisDB::Close() {
  delete t_client;
  t_client = nullptr;
}

RedisClient &RedisDB::client() {
  return *t_client;
}

int RedisDB::Read(const string &table, const string &key,
         const vector<string> *fields,
         vector<KVPair> &result) {
  if (fields) {
    int argc = fields->size() + 2;
    const char *argv[argc];
    size_t argvlen[argc];
    int i = 0;
    argv[i] = "HMGET"; argvlen[i] = strlen(argv[i]);
    argv[++i] = key.c_str(); argvlen[i] = key.length();
    for (const string &f : *fields) {
      argv[++i] = f.data(); argvlen[i] = f.size();
    }
    assert(i == argc - 1);
    redisReply *reply = (redisReply *)redisCommandArgv(
        client().context(), argc, argv, argvlen);
    if (!reply) return DB::kOK;
    assert(reply->type == REDIS_REPLY_ARRAY);
    assert(fields->size() == reply->elements);
    for (size_t i = 0; i < reply->elements; ++i) {
      const char *value = reply->element[i]->str;
      result.push_back(make_pair(fields->at(i), string(value ? value : "")));
    }
    freeReplyObject(reply);
  } else {
    redisReply *reply = (redisReply *)redisCommand(client().context(),
        "HGETALL %s", key.c_str());
    if (!reply) return DB::kOK;
    assert(reply->type == REDIS_REPLY_ARRAY);
    for (size_t i = 0; i < reply->elements / 2; ++i) {
      result.push_back(make_pair(
          string(reply->element[2 * i]->str),
          string(reply->element[2 * i + 1]->str)));
    }
    freeReplyObject(reply);
  }
  return DB::kOK;
}

int RedisDB::Update(const string &table, const string &key,
           vector<KVPair> &values) {
  string cmd("HMSET");
  size_t len = cmd.length() + key.length() + 1;
  for (KVPair &p : values) {
    len += p.first.length() + p.second.length() + 2;
  }
  cmd.reserve(len);

  cmd.append(" ").append(key);
  for (KVPair &p : values) {
    assert(p.first.find(' ') == string::npos);
    cmd.append(" ").append(p.first);
    assert(p.second.find(' ') == string::npos);
    cmd.append(" ").append(p.second);
  }
  assert(cmd.length() == len);
  client().Command(cmd);
  return DB::kOK;
}

namespace {
const char kIndexKey[] = "_indices";

/* Java's String.hashCode(), YCSB's score for a key in the index. */
string IndexScore(const string &key) {
  int32_t h = 0;
  for (unsigned char c : key) h = (int32_t)(31u * (uint32_t)h + c);
  return to_string(h);
}
}

int RedisDB::Insert(const string &table, const string &key,
           vector<KVPair> &values) {
  Update(table, key, values);
  client().Command(string("ZADD ") + kIndexKey + " " + IndexScore(key) + " " + key);
  return DB::kOK;
}

int RedisDB::Delete(const string &table, const string &key) {
  client().Command("DEL " + key);
  client().Command(string("ZREM ") + kIndexKey + " " + key);
  return DB::kOK;
}

int RedisDB::Scan(const string &table, const string &key, int len,
         const vector<string> *fields,
         vector<vector<KVPair>> &result) {
  redisReply *reply = (redisReply *)redisCommand(client().context(),
      "ZRANGEBYSCORE %s %s +inf LIMIT 0 %d", kIndexKey, IndexScore(key).c_str(), len);
  if (!reply) return DB::kOK;
  assert(reply->type == REDIS_REPLY_ARRAY);
  for (size_t i = 0; i < reply->elements; ++i) {
    result.emplace_back();
    Read(table, string(reply->element[i]->str, reply->element[i]->len), fields, result.back());
  }
  freeReplyObject(reply);
  return DB::kOK;
}

} // namespace ycsbc
