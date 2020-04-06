#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hiredis/hiredis.h>

int main(int argc, char **argv) {
    unsigned int j, isunix = 0;
    redisContext *c;
    redisReply *reply;
    const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";

    if (argc > 2) {
        if (*argv[2] == 'u' || *argv[2] == 'U') {
            isunix = 1;
            /* in this case, host is the path to the unix socket */
            printf("Will connect to unix socket @%s\n", hostname);
        }
    }

    int port = (argc > 2) ? atoi(argv[2]) : 11211;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    if (isunix) {
        c = redisConnectUnixWithTimeout(hostname, timeout);
    } else {
        c = redisConnectWithTimeout(hostname, port, timeout);
    }
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    // Run JavaScript function 'setup' to setup test data, 
    // including Facebook social graphs and neural network model. 
    reply = redisCommand(c,"JS %s", "setup");
    printf("JS setup: %s\n", reply->str);
    freeReplyObject(reply);

    // Run JavaScript function 'get' to read value of key 0,
    // which is the friend list of user id 0 in Facebook socail graph
    reply = redisCommand(c,"JS %s %s", "get", "0");
    printf("JS get 0: %s\n", reply->str);
    freeReplyObject(reply);

    // count_friend_list is a graph traversal function which counts a user's
    // friends and friends's friends and so on. The first parameter is the 
    // user ID and the second is the depth of the traversal.
    reply = redisCommand(c,"JS %s %s %s", "count_friend_list", "0", "1");
    printf("JS count_friend_list 0 1: %s\n", reply->str);
    freeReplyObject(reply);

    // Run JavaScript neural predict function, the result should be 1
    reply = redisCommand(c,"JS %s", "predict");
    printf("JS predict: %s\n", reply->str);
    freeReplyObject(reply);

    reply = redisCommand(c,"JS %s", "test");
    printf("JS test: %s\n", reply->str);
    freeReplyObject(reply);

    /* Disconnects and frees the context */
    redisFree(c);

    return 0;
}
