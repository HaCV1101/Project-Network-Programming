#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

char my_username[USERNAME_SIZE];
int my_socket;

int connect_to_server()
{
    int client_socket;
    struct sockaddr_in server_addr;

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        report_err(ERR_SOCKET_INIT);
        exit(0);
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        report_err(ERR_CONNECT_TO_SERVER);
        exit(0);
    }

    return client_socket;
}

void login_menu()
{
    printf("------ USER MANAGEMENT PROGRAM ------\n");
    printf("1. Register\n");
    printf("2. Activate\n");
    printf("3. Sign in\n");
    printf("4. Exit\n");
}

void user_menu()
{
    printf("****** Chat Room ******\n");
    printf("1. Private chat\n");
    printf("2. Group chat\n");
    printf("3. Show online users\n");
    printf("4. Search users\n");
    printf("5. Change password\n");
    printf("6. Sign out\n");
}

void ask_server(int client_socket)
{
    int choice, result;
    Package pkg;

    while (1)
    {
        login_menu();
        printf("Your choice: ");
        scanf("%d", &choice);
        clear_stdin_buff();

        switch (choice)
        {
        case 1:
        {
            pkg.ctrl_signal = REGISTER_REQ;
            send(client_socket, &pkg, sizeof(pkg), 0);
            result = signup(client_socket);
            if (result == REGISTER_SUCC)
            {
                printf("Successful Registration\n");
            }
            else
            {
                printf("Account existed\n");
            }
            break;
        }
        case 2:
        {
            pkg.ctrl_signal = ACTIVATE_REQ;
            send(client_socket, &pkg, sizeof(pkg), 0);
            result = activate(client_socket);
            if (result == ACTIVATE_SUCC)
            {
                printf("Successful Activation\n");
            }
            else if (result == ACC_BLOCKED)
            {
                printf("This account is blocked\n");
            }
            else if (result == ACC_ACTIVATED)
            {
                printf("This account is activated\n");
            }
            else
                printf("Activation Fail\n");
            break;
        }
        case 3:
            pkg.ctrl_signal = LOGIN_REQ;
            send(client_socket, &pkg, sizeof(pkg), 0);
            result = login(client_socket);
            if (result == LOGIN_SUCC)
            {
                user_use(client_socket);
            }
            else if (result == ACC_BLOCKED)
            {
                printf("This account is blocked\n");
            }
            else if (result == ACC_IDLE)
            {
                printf("This account isn't activated\n");
            }
            else if (result == INCORRECT_ACC)
            {
                report_err(ERR_INCORRECT_ACC);
            }
            else
            {
                report_err(ERR_SIGNED_IN_ACC);
            }
            break;
        case 4:
            pkg.ctrl_signal = QUIT_REQ;
            send(client_socket, &pkg, sizeof(pkg), 0);
            exit(0);
        }
    }
}
int signup(int client_socket)
{
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    Package pkg;

    printf("Username: ");
    scanf("%s", username);
    clear_stdin_buff();

    printf("Password: ");
    scanf("%s", password);
    clear_stdin_buff();

    strcpy(pkg.msg, username);
    send(client_socket, &pkg, sizeof(pkg), 0);

    recv(client_socket, &pkg, sizeof(pkg), 0);

    strcpy(pkg.msg, password);
    send(client_socket, &pkg, sizeof(pkg), 0);

    recv(client_socket, &pkg, sizeof(pkg), 0);

    return pkg.ctrl_signal;
}

int activate(int client_socket)
{
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    char capcha[10];
    Package pkg;

    printf("Username: ");
    scanf("%s", username);
    clear_stdin_buff();

    printf("Password: ");
    scanf("%s", password);
    clear_stdin_buff();

    strcpy(pkg.msg, username);
    send(client_socket, &pkg, sizeof(pkg), 0);

    recv(client_socket, &pkg, sizeof(pkg), 0);

    strcpy(pkg.msg, password);
    send(client_socket, &pkg, sizeof(pkg), 0);

    recv(client_socket, &pkg, sizeof(pkg), 0);

    if (pkg.ctrl_signal == ACC_BLOCKED || pkg.ctrl_signal == ACC_ACTIVATED)
    {
        return pkg.ctrl_signal;
    }
    printf("Activate Code: %s\n", pkg.msg);
    printf("Input Activate Code: ");
    scanf("%s", capcha);
    clear_stdin_buff();

    strcpy(pkg.msg, capcha);
    send(client_socket, &pkg, sizeof(pkg), 0);

    recv(client_socket, &pkg, sizeof(pkg), 0);

    return pkg.ctrl_signal;
}

int login(int client_socket)
{

    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    Package pkg;

    printf("Username: ");
    scanf("%s", username);
    clear_stdin_buff();

    printf("Password: ");
    scanf("%s", password);
    clear_stdin_buff();

    strcpy(pkg.msg, username);
    send(client_socket, &pkg, sizeof(pkg), 0);

    recv(client_socket, &pkg, sizeof(pkg), 0);

    strcpy(pkg.msg, password);
    send(client_socket, &pkg, sizeof(pkg), 0);

    recv(client_socket, &pkg, sizeof(pkg), 0);
    if (pkg.ctrl_signal == LOGIN_SUCC)
        strcpy(my_username, username);
    return pkg.ctrl_signal;
}

void user_use(int client_socket)
{
    printf("Login successfully!\n");
    int login = 1;
    int choice, result;
    Package pkg;

    pthread_t read_st;
    if (pthread_create(&read_st, NULL, read_msg, (void *)&client_socket) < 0)
    {
        report_err(ERR_CREATE_THREAD);
        exit(0);
    }

    see_active_user(client_socket);

    while (1)
    {
        user_menu();
        printf("Your choice: \n");
        scanf("%d", &choice);
        clear_stdin_buff();

        switch (choice)
        {
        case 1: // private chat
            private_chat(client_socket);
            break;
        case 2: // group chat
        {
            group_chat(client_socket);
            break;
        }

        case 3: // show online users
            see_active_user(client_socket);
            break;

        case 4: // search
        {
            search_users(client_socket);
            break;
        }
        case 5: // change password
        {
            change_password(client_socket);
            break;
        }
        case 6: // sign out
            login = 0;
            pkg.ctrl_signal = LOG_OUT;
            send(client_socket, &pkg, sizeof(pkg), 0);
            strcpy(my_username, "");
            break;
        }
        if (login == 0)
            break;
    }
    pthread_detach(read_st);
}

void search_users(int client_socket)
{
}

void change_password(int client_socket)
{
    char password[PASSWORD_SIZE];
    char new_password[PASSWORD_SIZE];
    Package *pkg;
    while (1)
    {
        printf("Password: ");
        scanf("%s", password);
        clear_stdin_buff();

        strcpy(pkg->msg, password);
        send(client_socket, &pkg, sizeof(pkg), 0);

        recv(client_socket, &pkg, sizeof(pkg), 0);
        if (pkg->ctrl_signal == CORRECT_PASSWORD)
        {
            break;
        }
        printf("Wrong Password. Please Input Your Password.\n");
    }
    printf("Input New Password: ");
    scanf("%s", new_password);
    clear_stdin_buff();

    strcpy(pkg->msg, new_password);
    send(client_socket, &pkg, sizeof(pkg), 0);

    recv(client_socket, &pkg, sizeof(pkg), 0);
    if (pkg->ctrl_signal == CHANGE_PASSWORD_SUCC)
    {
        printf("Password Changed Successfully\n");
    }
    else
    {
        printf("Password and Newpassword Are The Same\n");
    }
}

void *read_msg(void *param)
{
    int *c_socket = (int *)param;
    int client_socket = *c_socket;
    // printf("\nmysoc: %d\n", client_socket);
    // int client_socket = my_socket;
    Package pkg;
    while (1)
    {
        recv(client_socket, &pkg, sizeof(pkg), 0);
        // printf("receive %d from server\n", pkg.ctrl_signal);
        switch (pkg.ctrl_signal)
        {
            // case REGISTER_SUCC:
            //     printf(" Successful Registration\n");
            //     break;

            // case ACC_EXISTED:
            //     printf("This account existed!!!\n");
            //     break;

        case ACTIVATE_SUCC:
            printf("Successfull Activation\n");
            break;

        case ACTIVATE_FAIL:
            printf("Activation failed!!!\n");

        case SHOW_USER:
            printf("Current online users: %s \n", pkg.msg);
            break;

        case PRIVATE_CHAT:
            printf("%s: %s\n", pkg.sender, pkg.msg);
            break;

        case ERR_INVALID_RECEIVER:
            report_err(ERR_INVALID_RECEIVER);
            break;

        case MSG_SENT_SUCC:
            printf("Message sent!\n");
            break;

        default:
            break;
        }
    }
}

void see_active_user(int client_socket)
{
    Package pkg;
    pkg.ctrl_signal = SHOW_USER;
    send(client_socket, &pkg, sizeof(pkg), 0);

    sleep(1);

    // recv(client_socket, &pkg, sizeof(pkg), 0);
}

void private_chat(int client_socket)
{
    Package pkg;
    char username[USERNAME_SIZE];
    char msg[MSG_SIZE];
    pkg.ctrl_signal = PRIVATE_CHAT;
    // send(client_socket, &pkg, sizeof(pkg), 0);

    printf("Receiver: \n");
    fgets(username, USERNAME_SIZE, stdin);
    username[strlen(username) - 1] = '\0';
    strcpy(pkg.receiver, username);
    strcpy(pkg.sender, my_username);
    // send(client_socket, &pkg, sizeof(pkg), 0);

    while (1)
    {
        printf("Message(leave blank to exit private chat): \n");
        fgets(msg, MSG_SIZE, stdin);
        msg[strlen(msg) - 1] = '\0';
        if (strlen(msg) == 0)
        {
            break;
        }

        strcpy(pkg.msg, msg);
        send(client_socket, &pkg, sizeof(pkg), 0);

        sleep(1);
    }
}

void group_chat(int client_socket)
{
}

int main()
{
    int client_socket = connect_to_server();
    my_socket = client_socket;
    ask_server(client_socket);
    close(client_socket);
    return 0;
}