//
// A C++ Redis client that wraps hiredis
//

#ifndef YCSB_C_REDIS_CLIENT_H_
#define YCSB_C_REDIS_CLIENT_H_

#include <iostream>
#include <string>
#include "redis/hiredis/hiredis.h"
#include "redis/hiredis/hiredis_ssl.h"

namespace ycsbc {

class RedisClient {
 public:
  RedisClient(const char *host, int port, int slaves);
  ~RedisClient();

  int Command(std::string cmd);

  redisContext *context() { return context_; }
 private:
  void HandleError(redisReply *reply, const char *hint);

  redisSSLContext *ssl;
  redisSSLContextError ssl_error;
  redisContext *context_;
  int slaves_;
};

//
// Implementation
//
inline RedisClient::RedisClient(const char *host, int port, int slaves) :
    slaves_(slaves) {
  if (port == 8886) {
    context_ = redisConnectHoma(host, port);
  }
  else if (port == 8887) {
    context_ = redisConnectHomaLs(host, port);
  }
  else {
    if (port == 8889) {
      redisInitOpenSSL();
      redisSSLOptions options;
      options.cacert_filename = "./tls/ca.crt";
      options.cert_filename = "./tls/redis.crt";
      options.private_key_filename = "./tls/redis.key";
      options.capath = NULL;
      options.server_name = NULL;
      options.verify_mode = REDIS_SSL_VERIFY_NONE;
  
      ssl = redisCreateSSLContextWithOptions(&options, &ssl_error);
      if (ssl == NULL) {
          printf("SSL Context error: %s\n",
                  redisSSLContextGetError(ssl_error));
          exit(1);
      }
    }
    context_ = redisConnect(host, port);
  }

  if (!context_ || context_->err) {
    if (context_) {
      std::cerr << "Connect error: " << context_->errstr << std::endl;
      redisFree(context_);
    } else {
      std::cerr << "Connect error: can't allocate redis context!" << std::endl;
    }
    exit(1);
  }

  if (port == 8889) {
    if (redisInitiateSSLWithContext(context_, ssl) != REDIS_OK) {
        printf("Couldn't initialize SSL!\n");
        printf("Error: %s\n", context_->errstr);
        redisFree(context_);
        exit(1);
    }
  }
}

inline RedisClient::~RedisClient() {
  if (context_) {
    redisFree(context_);
  }
}

inline int RedisClient::Command(std::string cmd) {
  redisReply *reply;
  redisAppendCommand(context_, cmd.data());
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
