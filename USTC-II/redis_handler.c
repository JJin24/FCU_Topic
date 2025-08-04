#include "redis_handler.h"

redisContext* redis_connect(const char* host, int port) {
    redisContext *c = redisConnect(host, port);
    if (c == NULL || c->err) {
        if (c) {
            fprintf(stderr, "Redis Connection Error: %s\n", c->errstr);
            redisFree(c);
        } else {
            fprintf(stderr, "Can't allocate redis context\n");
        }
        return NULL;
    }
    printf("Successfully connected to Redis.\n");
    return c;
}

void redis_disconnect(redisContext* c) {
    if (c != NULL) {
        redisFree(c);
        printf("Disconnected from Redis.\n");
    }
}