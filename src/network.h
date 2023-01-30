#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "account_manager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MSG_SIZE 2048

//* Tín hiệu điều khiển
// chung

// server
#define REGISTER_SUCC 100
#define ACC_EXISTED -100
#define ACTIVATE_SUCC 101
#define ACTIVATE_FAIL -101
#define LOGIN_SUCC 102
#define INCORRECT_ACC -102
#define SIGNED_IN_ACC 103
#define RECV_SUCC 104
#define ACC_BLOCKED 0
#define ACC_IDLE 2
#define ACC_ACTIVATED 1
#define CHANGE_PASSWORD_SUCC 8
#define CHANGE_PASSWORD_FAIL -8
#define WRONG_PASSWORD -10
#define CORRECT_PASSWORD 10

// client
#define REGISTER_REQ 200
#define ACTIVATE_REQ 201
#define LOGIN_REQ 202
#define SEARCH_REQ 203
#define CHANGE_PASS_REQ 204
#define QUIT_REQ 205

#define MSG_SENT_SUCC 302
#define END_CHAT 312

#define SHOW_USER 401
#define PRIVATE_CHAT 402
#define GROUP_CHAT 403
#define SEARCH_USERS 404
#define CHANGE_PASSWORD 405
#define LOG_OUT 406

//* Cấu trúc gói tin
typedef struct Package_
{
    char msg[MSG_SIZE];           /* nội dung thông điệp */
    char sender[USERNAME_SIZE];   /* username người gửi */
    char receiver[USERNAME_SIZE]; /* username người nhận */
    int ctrl_signal;              /* mã lệnh */
} Package;

#endif