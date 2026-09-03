//
// A C++ Redis client that wraps hiredis
//

#ifndef YCSB_C_REDIS_CLIENT_H_
#define YCSB_C_REDIS_CLIENT_H_

#include <iostream>
#include <string>
#include <vector>
#include "redis/hiredis/hiredis.h"

namespace ycsbc {

class RedisClient {
 public:
  RedisClient(const char *host, int port, int slaves,
              const std::string &transport = "tcp");
  ~RedisClient();

  int Command(std::string cmd);

  redisContext *context() { return context_; }
 private:
  void HandleError(redisReply *reply, const char *hint);

  redisContext *context_;
  int slaves_;
};

//
// Implementation
//
inline RedisClient::RedisClient(const char *host, int port, int slaves,
                                const std::string &transport) :
    slaves_(slaves) {
  context_ = (transport == "homa") ? redisConnectHoma(host, port)
                                   : redisConnect(host, port);
  if (!context_ || context_->err) {
    if (context_) {
      std::cerr << "Connect error: " << context_->errstr << std::endl;
      redisFree(context_);
    } else {
      std::cerr << "Connect error: can't allocate redis context!" << std::endl;
    }
    exit(1);
  }
}

inline RedisClient::~RedisClient() {
  if (context_) {
    redisFree(context_);
  }
}

inline int RedisClient::Command(std::string cmd) {
  redisReply *reply;
  /* Split on spaces and send via argv so field values containing '%' (or other
   * printf specifiers) can't be misread as a format string / corrupt the heap.
   * YCSB tokens contain no embedded spaces. */
  std::vector<std::string> toks;
  for (size_t i = 0, j; i < cmd.size(); i = j + 1) {
    j = cmd.find(' ', i);
    if (j == std::string::npos) j = cmd.size();
    if (j > i) toks.emplace_back(cmd.substr(i, j - i));
  }
  std::vector<const char *> argv(toks.size());
  std::vector<size_t> argvlen(toks.size());
  for (size_t i = 0; i < toks.size(); i++) {
    argv[i] = toks[i].data();
    argvlen[i] = toks[i].size();
  }
  redisAppendCommandArgv(context_, (int)argv.size(), argv.data(), argvlen.data());
  if (slaves_) {
    redisAppendCommand(context_, "WAIT %d %d", slaves_, 0);
  }
  if (redisGetReply(context_, (void **)&reply) == REDIS_ERR) {
    HandleError(reply, cmd.c_str());
  }
  freeReplyObject(reply);
  if (slaves_) {
    if (redisGetReply(context_, (void **)&reply) == REDIS_ERR) {
      HandleError(reply, "WAIT");
    }
    freeReplyObject(reply);
  }
  return 0;
}

inline void RedisClient::HandleError(redisReply *reply, const char *hint) {
  std::cerr << hint << " error: " << context_->errstr << std::endl;
  if (reply) freeReplyObject(reply);
  redisFree(context_);
  exit(2); 
}

} // namespace ycsbc

#endif // YCSB_C_REDIS_CLIENT_H_
