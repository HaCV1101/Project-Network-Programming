#include "server.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

Active_user user[MAX_USER];

int create_listen_socket()
{

    int listen_socket;
    struct sockaddr_in server_addr;

    if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        report_err(ERR_SOCKET_INIT);
        exit(0);
    }

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        report_err(ERR_SOCKET_INIT);
        exit(0);
    }

    if (listen(listen_socket, MAX_USER) < 0)
    {
        report_err(ERR_SOCKET_INIT);
        exit(0);
    }

    return listen_socket;
}

int accept_conn(int listen_socket)
{

    int conn_socket;
    struct sockaddr_in client_addr;
    int client_addr_size = sizeof(struct sockaddr);

    if ((conn_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &client_addr_size)) < 0)
    {
        report_err(ERR_CONN_ACCEPT);
        exit(0);
    }

    return conn_socket;
}

void make_server()
{

    Account *acc_list;
    int listen_socket;

    acc_list = read_account_list();
    listen_socket = create_listen_socket();

    while (1)
    {

        int conn_socket = accept_conn(listen_socket);

        Login_req req;
        req.conn_socket = conn_socket;
        req.acc_list = acc_list;

        pthread_t client_thr;
        if (pthread_create(&client_thr, NULL, pre_login_srv, (void *)&req) < 0)
        {
            report_err(ERR_CREATE_THREAD);
            exit(0);
        }
        pthread_detach(client_thr);
    }

    close(listen_socket);
}

void *pre_login_srv(void *param)
{

    Login_req *req = (Login_req *)param;
    Package pkg;

    while (1)
    {
        recv(req->conn_socket, &pkg, sizeof(pkg), 0);

        switch (pkg.ctrl_signal)
        {
        case REGISTER_REQ:
            handle_signup(req->conn_socket, req->acc_list);
            break;
        case ACTIVATE_REQ:
            handle_activate(req->conn_socket, req->acc_list);
            break;
        case LOGIN_REQ:
            handle_login(req->conn_socket, req->acc_list);
            break;
        case QUIT_REQ:
            printf("user quit\n");
        }

        if (pkg.ctrl_signal == QUIT_REQ)
        {
            break;
        }
    }
}

void handle_signup(int conn_socket, Account *acc_list)
{

    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    Package pkg;
    Account *target_acc;
    int result;

    recv(conn_socket, &pkg, sizeof(pkg), 0);
    strcpy(username, pkg.msg);

    pkg.ctrl_signal = RECV_SUCC;
    send(conn_socket, &pkg, sizeof(pkg), 0);

    recv(conn_socket, &pkg, sizeof(pkg), 0);
    strcpy(password, pkg.msg);

    printf("%s\n", username);
    printf("%s\n", password);

    target_acc = find_account(acc_list, username);
    if (target_acc)
    {
        result = ACC_EXISTED;
    }
    else
    {
        result = REGISTER_SUCC;
        target_acc = acc_list;
        while (target_acc)
        {
            if (target_acc->next == NULL)
                break;
            target_acc = target_acc->next;
        }
        Account *currAcc = target_acc;
        target_acc = target_acc->next;
        target_acc = (Account *)malloc(sizeof(Account));
        strcpy(target_acc->username, username);
        strcpy(target_acc->password, password);
        target_acc->status = 2;
        target_acc->consecutive_failed_sign_in = 0;
        target_acc->is_signed_in = 0;
        target_acc->next = NULL;
        currAcc->next = target_acc;
        write_to_file(acc_list);
    }

    pkg.ctrl_signal = result;
    send(conn_socket, &pkg, sizeof(pkg), 0);
    printf("%d \n", pkg.ctrl_signal);
}

void handle_activate(int conn_socket, Account *acc_list)
{
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    char capcha[10];
    char capchaClient[10];
    Package pkg;
    Account *target_acc;
    int result;

    recv(conn_socket, &pkg, sizeof(pkg), 0);
    strcpy(username, pkg.msg);

    pkg.ctrl_signal = RECV_SUCC;
    send(conn_socket, &pkg, sizeof(pkg), 0);

    recv(conn_socket, &pkg, sizeof(pkg), 0);
    strcpy(password, pkg.msg);

    printf("%s\n", username);
    printf("%s\n", password);
    target_acc = find_account(acc_list, username);
    if (target_acc)
    {
        if (strcmp(target_acc->password, password) == 0)
        {
            if (target_acc->status == 2)
                result = ACC_IDLE;
            else if (target_acc->status == 0)
            {
                result = ACC_BLOCKED;
            }
            else
            {
                result = ACC_ACTIVATED;
            }
        }
        else
        {
            result = INCORRECT_ACC;
        }
    }

    else
    {
        result = INCORRECT_ACC;
    }
    if (result == ACC_IDLE)
    {
        ranCapcha(capcha);
        pkg.ctrl_signal = result;
        printf("%s %ld\n", capcha, strlen(capcha));
        strcpy(pkg.msg, capcha);
        send(conn_socket, &pkg, sizeof(pkg), 0);
        recv(conn_socket, &pkg, sizeof(pkg), 0);

        strcpy(capchaClient, pkg.msg);

        printf("%s %ld\n", capchaClient, strlen(capchaClient));
        int x = strcmp(capcha, capchaClient);
        // printf("%d\n", x);

        if (!x)
        {
            target_acc->status = 1;
            pkg.ctrl_signal = ACTIVATE_SUCC;
            write_to_file(acc_list);
        }
        else
            pkg.ctrl_signal = ACTIVATE_FAIL;
        send(conn_socket, &pkg, sizeof(pkg), 0);
    }
    else
    {
        send(conn_socket, &pkg, sizeof(pkg), 0);
    }
}

void handle_login(int conn_socket, Account *acc_list)
{

    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    Package pkg;
    Account *target_acc;
    int result;

    recv(conn_socket, &pkg, sizeof(pkg), 0);
    strcpy(username, pkg.msg);

    pkg.ctrl_signal = RECV_SUCC;
    send(conn_socket, &pkg, sizeof(pkg), 0);

    recv(conn_socket, &pkg, sizeof(pkg), 0);
    strcpy(password, pkg.msg);

    printf("%s\n", username);
    printf("%s\n", password);

    target_acc = find_account(acc_list, username);
    if (target_acc)
    {
        if (target_acc->is_signed_in)
        {
            result = SIGNED_IN_ACC;
        }
        else
        {
            if (strcmp(target_acc->password, password) == 0)
            {
                if (target_acc->status == 1)
                    result = LOGIN_SUCC;
                else if (target_acc->status == 0)
                {
                    result = ACC_BLOCKED;
                }
                else
                {
                    result = ACC_IDLE;
                }
            }
            else
            {
                result = INCORRECT_ACC;
            }
        }
    }
    else
    {
        result = INCORRECT_ACC;
    }

    if (result == LOGIN_SUCC)
    {
        printf("Login Success\n");
        target_acc->is_signed_in = 1;

        for (int i = 0; i < MAX_USER; i++)
        {
            if (user[i].socket == 0)
            {
                strcpy(user[i].username, username);
                user[i].socket = conn_socket;
                break;
            }
        }
    }
    else if (result == SIGNED_IN_ACC)
    {
        printf("Already Signed In Account\n");
    }
    else
    {
        printf("Incorrect Account\n");
    }

    pkg.ctrl_signal = result;
    send(conn_socket, &pkg, sizeof(pkg), 0);
    if (result == LOGIN_SUCC)
        sv_user_use(conn_socket);
}

void sv_user_use(int conn_socket)
{

    Package pkg;
    Account acc_list;
    int login = 1;
    while (1)
    {
        if (recv(conn_socket, &pkg, sizeof(pkg), 0) > 0)
            printf("Receive from %d\n", conn_socket);
        printf("%d chooses %d \n", conn_socket, pkg.ctrl_signal);
        switch (pkg.ctrl_signal)
        {
        case PRIVATE_CHAT:
            sv_private_chat(conn_socket, &pkg);
            break;

        case GROUP_CHAT:

            break;
        case SHOW_USER:
            sv_active_user(conn_socket, &pkg);
            break;

        case CHANGE_PASSWORD:
            sv_change_password(conn_socket, &pkg, &acc_list);
            break;

        case LOG_OUT:
            login = 0;
            printf("%d logout\n", conn_socket);
            break;

        default:
            break;
        }
        printf("Done %d of %d\n", pkg.ctrl_signal, conn_socket);
        if (login == 0)
            break;
    }

    for (int i = 0; i < MAX_USER; i++)
    {
        if (user[i].socket == conn_socket)
        {
            user[i].socket = 0;
            break;
        }
    }
}

void sv_active_user(int conn_socket, Package *pkg)
{

    char user_list[MSG_SIZE] = {0};
    for (int i = 0; i < MAX_USER; i++)
    {
        if (user[i].socket != 0)
        {
            strcat(user_list, user[i].username);
            int len = strlen(user_list);
            user_list[len] = ' ';
        }
    }
    strcpy(pkg->msg, user_list);
    send(conn_socket, pkg, sizeof(*pkg), 0);
}

void sv_private_chat(int conn_socket, Package *pkg)
{

    printf("%d: %s to %s: %s\n", pkg->ctrl_signal, pkg->sender, pkg->receiver, pkg->msg);

    int i = 0;

    for (i = 0; i < MAX_USER; i++)
    {
        if (strcmp(pkg->receiver, user[i].username) == 0 && user[i].socket != 0)
        {
            // recv_socket = user[i].socket;
            send(user[i].socket, pkg, sizeof(*pkg), 0);
            break;
        }
    }

    if (i == MAX_USER)
        pkg->ctrl_signal = ERR_INVALID_RECEIVER;
    else
        pkg->ctrl_signal = MSG_SENT_SUCC;

    send(conn_socket, pkg, sizeof(*pkg), 0);
}

void sv_search(int conn_socket, Package *pkg)
{
}

void sv_change_password(int conn_socket, Package *pkg, Account *acc_list)
{
    char password[PASSWORD_SIZE];
    char new_password[PASSWORD_SIZE];
    Account *acc;
    for (int i = 0; i < MAX_USER; i++)
    {
        if (user[i].socket == conn_socket)
        {
            acc = find_account(acc_list, user[i].username);
            break;
        }
    }
    while (1)
    {
        recv(conn_socket, pkg, sizeof(*pkg), 0);
        if (strcmp(pkg->msg, acc->password) == 0)
        {
            strcpy(password, pkg->msg);
            pkg->ctrl_signal = CORRECT_PASSWORD;
            send(conn_socket, pkg, sizeof(*pkg), 0);
            break;
        }
        pkg->ctrl_signal = WRONG_PASSWORD;
        send(conn_socket, pkg, sizeof(*pkg), 0);
    }
    recv(conn_socket, pkg, sizeof(*pkg), 0);
    strcpy(new_password, pkg->msg);

    if (strcmp(password, new_password) != 0)
    {
        pkg->ctrl_signal = CHANGE_PASSWORD_SUCC;
    }
    else
    {
        pkg->ctrl_signal = CHANGE_PASSWORD_FAIL;
    }
    send(conn_socket, pkg, sizeof(*pkg), 0);
}

int main()
{
    make_server();
    return 0;
}