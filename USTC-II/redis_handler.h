#ifndef REDIS_HANDLER_H
#define REDIS_HANDLER_H

#include "common.h"

redisContext* redis_connect(const char* host, int port);
void redis_disconnect(redisContext* c);

#endif // REDIS_HANDLER_H