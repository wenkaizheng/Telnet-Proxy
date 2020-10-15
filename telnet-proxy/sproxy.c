//
//  Sporxy.c
//  Created by  Wenkai Zheng <wenkaizheng@email.arizona.edu>
//  Description: Implement TCP proxy program.

#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/select.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "network.h"
#define port_telent 23
#define BACKLOG 5
// used for cproxy and sproxy
struct sockaddr_in local_addr_info = {0};
struct sockaddr_in remote_addr_indo = {0};
int socket_cproxy = 0;
int port_cproxy = 0;
char buff_cproxy[1] = {0};
int accpet_cproxy = 0;
socklen_t socket_length = 0;
int recv_length = 0;

// used for sproxy and telnet
struct sockaddr_in sproxy_telent_addr_info = {0};
int socket_telent = 0;
char buff_telent[1] = {0};
int connect_telent = 0;

// new Data package
char confirm[6] = {0};
// used for heartbeat package
char heartbeat[6] = {0};
char data[6] = {0};
char data_2[6] = {0};

// determeined for need reconnection or not
int reconnect = 0;
int change_ip = 0;
int retransfer = 0;
int count_time = 0;
int recon_telnet = 0;
int sp_exit = 0;
int network_exp = 0;

// time varible
struct timeval time_current;
struct timeval time_heartbeat_send;
struct timeval time_heartbeat_recv;
struct timeval time_result;
struct timeval time_retransfer_send;
struct timeval time_retransfer_current;

// queue data structure
node *head = NULL;
node *tail = NULL;

// my ack number
int number = 0;
// cproxy ack number
int accept_ackNum = -1;
int last_send = 0;
char last_data[1] = {0};
// init as clinet for telnet
void telnet_connection()
{
    sproxy_telent_addr_info.sin_family = AF_INET;
    sproxy_telent_addr_info.sin_port = htons(port_telent);
    socket_telent = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_telent < 0)
    {
        fprintf(stderr, "Creation of socket failed between sproxy and telnet.\n");
        exit(1);
    }
    int rc = inet_pton(AF_INET, "127.0.0.1", &(sproxy_telent_addr_info.sin_addr));
    if (rc <= 0)
    {
        fprintf(stderr, "Invalid ip address between sproxy and telnet.\n");
        exit(1);
    }
    rc = connect(socket_telent, (struct sockaddr *)&sproxy_telent_addr_info, sizeof(sproxy_telent_addr_info));
    if (rc != 0)
    {
        fprintf(stderr, "Connect error between sproxy and telnet.\n");
        exit(1);
    }
}

void send_message(int flag, node *h)
{
    fd_set writefds;
    // we need to send heart message
    if (flag == 0)
    {
        heartbeat[0] = 'h';
        heartbeat[1] = '0';
        heartbeat[2] = '-';
        heartbeat[3] = '~';
        heartbeat[4] = '~';
        heartbeat[5] = '~';

        // send data
    }
    else if (flag == 1)
    {
        data_2[0] = 'd';
        data_2[1] = h->ack & 0xff;
        data_2[2] = (h->ack >> 8) & 0xff;
        data_2[3] = (h->ack >> 16) & 0xff;
        data_2[4] = (h->ack >> 24) & 0xff;
        data_2[5] = *(h->a);

        // send confirm
    }
    else if (flag == 2)
    {
        confirm[0] = 'a';
        confirm[1] = data[1];
        confirm[2] = data[2];
        confirm[3] = data[3];
        confirm[4] = data[4];
        confirm[5] = '~';
        confirm[5] = '~';
    }else if(flag == 3){
        data_2[0] = 'r';
        data_2[1] = data[1];
        data_2[2] = data[2];
        data_2[3] = data[3];
        data_2[4] = data[4];
        data_2[5] = '~';
    }

    FD_ZERO(&writefds);
    // add our descriptors to the set
    FD_SET(accpet_cproxy, &writefds);
    int m = accpet_cproxy + 1;
    struct timeval tv1;
    tv1.tv_sec = 10;
    tv1.tv_usec = 0;
    int rv = select(m, NULL, &writefds, NULL, &tv1);
    if (rv == -1)
    {
        fprintf(stderr, "Select return -1 in select of send_heartbeat\n");
        // error occurred in select()
        exit(1);
    }
    else if (rv == 0)
    {
        // not sure but add printf to see
    }
    else
    {
        if (FD_ISSET(accpet_cproxy, &writefds))
        {
            if (flag == 0)
            {
                rv = send_all(accpet_cproxy, heartbeat, 6);
                gettimeofday(&time_heartbeat_send, NULL);
            }

            else if (flag == 1 || flag == 3)
            {
                rv = send_all(accpet_cproxy, data_2, 6);
                
            }
            else
            {
                rv = send_all(accpet_cproxy, confirm, 6);
            }
            // we need also send heatbeat message in here
            // send heartbeat message

            if (rv == -1)
            {
                    network_exp = 1;
            }
        }
    }
}

void clean_up()
{
    bzero(data_2, 6);
    bzero(data, 6);
    bzero(heartbeat, 6);
    bzero(buff_cproxy, 1);
    bzero(buff_telent, 1);
    bzero(confirm, 6);
}
void init()
{

    // first of all, init sproxy socket for cproxy
    if(retransfer ==0){
        node* p = NULL;
        while(head != NULL){
           pop(&p,&head,&tail);
        }
    }
    if (reconnect == 0 && change_ip == 0)
    {
        socket_cproxy = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_cproxy < 0)
        {
            fprintf(stderr, "Create socket fail between cproxy and sproxy");
            exit(1);
        }
        local_addr_info.sin_family = AF_INET;
        local_addr_info.sin_port = htons(port_cproxy);
        local_addr_info.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(socket_cproxy, (void *)&local_addr_info, sizeof(local_addr_info)) < 0)
        {
            fprintf(stderr, "401th bind fail between cproxy and sproxy\n");
            exit(1);
        }
    }

    //cproxy does not change ip and init listen() and accpet()
    if (change_ip == 0)
    {
        if (listen(socket_cproxy, BACKLOG) < 0)
        {
            fprintf(stderr, "listen fail between cproxy and sproxy 242th\n");
            exit(1);
        }
        accpet_cproxy = accept(socket_cproxy, (void *)&remote_addr_indo, &socket_length);
        if (accpet_cproxy < 0)
        {
            fprintf(stderr, "accpet fail between cproxy and sproxy\n");
            exit(1);
        }
        // if cp does not change ip and need reconnect to telnet
        telnet_connection();
    }
    else
    {
      //  printf("wait for Cproxy\n");
        if(recon_telnet == 0){
        if (listen(socket_cproxy, BACKLOG) < 0)
        {
            fprintf(stderr, "listen fail between cproxy and sproxy 260th\n");
            exit(1);
        }
        accpet_cproxy = accept(socket_cproxy, (void *)&remote_addr_indo, &socket_length);
        if (accpet_cproxy < 0)
        {
            fprintf(stderr, "accpet fail between cproxy and sproxy\n");
            exit(1);
        }
        }
    }
    
    // need to reconnet to telnet
    if(recon_telnet == 1){
        telnet_connection();
        recon_telnet = 0;
       // printf("Reconnect with telnet successfully\n");
    }
    
    change_ip = 0;
    sp_exit = 0;
    int n;
    fd_set readfds;
    fd_set writefds;
    while (1)
    {
        //printf("We got something in here\n");
        if (head == NULL && tail == NULL && sp_exit == 1)
        {
            close(socket_telent);
            close(accpet_cproxy);
            reconnect = 1;
            number = 0;
            retransfer = 0;
            clean_up();
            break;
        }

        // select()
        FD_ZERO(&readfds);
        FD_SET(accpet_cproxy, &readfds);
        FD_SET(socket_telent, &readfds);
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        n = accpet_cproxy > socket_telent ? accpet_cproxy + 1 : socket_telent + 1;
        int rv = select(n, &readfds, NULL, NULL, &tv);
        if (rv == -1)
        {
            fprintf(stderr, "select return -1 in 278th \n");
            exit(1);
        }
        // timeout for read means nothing do on telnet and sproxy
        // then check difference time for last recv hearbeat time and current time
        // if diff time > 3, means cproxy lost connection
        else if (rv == 0)
        {
            if (time_heartbeat_recv.tv_sec != 0)
            {
                gettimeofday(&time_current, NULL);
                timersub(&time_current, &time_heartbeat_recv, &time_result);
                if (time_result.tv_sec >= 3)
                {
                    close(accpet_cproxy);
                    change_ip = 1;
                    retransfer = 1;
                    clean_up();
                    break;
                }
            }
            // beganning of buliding the connection, first time send heartbeat
            send_message(0, NULL);
            if (network_exp == 1){
                 close(accpet_cproxy);
                 change_ip = 1;
                 retransfer = 1;
                 clean_up();
                 break;
            }
        }
        else
        {
            // as server for cproxy
            if (FD_ISSET(accpet_cproxy, &readfds))
            {

                // receive message from cproxy
                rv = recv_all(accpet_cproxy, data, 6);

                // When recv( ) returns 0, socket should be closed
                // Then will go back to the listenin
                if (rv == 0)
                {
                    close(socket_telent);
                    close(accpet_cproxy);
                    reconnect = 1;
                    retransfer = 0;
                    clean_up();
                    break;
                }
                if (rv == -1)
                {
                    close(accpet_cproxy);
                    change_ip = 1;
                    retransfer = 1;
                    clean_up();
                    break;
                }
                //
                if (data[0] == 'h' && data[1] == '0' && data[2] == '+')
                {
                    gettimeofday(&time_heartbeat_recv, NULL);
                    gettimeofday(&time_current, NULL);
                    timersub(&time_current, &time_heartbeat_send, &time_result);
                    if (time_result.tv_sec >= 1)
                    {
                        send_message(0, NULL);
                        if (network_exp == 1){
                             close(accpet_cproxy);
                             change_ip = 1;
                             retransfer = 1;
                             clean_up();
                             break;
                        }
                        gettimeofday(&time_heartbeat_send, NULL);
                    }
                    clean_up();
                    // continue;
                    // when package is not heartbeat
                    // dont need to record data package recevied time
                    // cause we send heartbeat every single second as FAQ file said
                }else if(data[0] == 'r'){
                    node* p = NULL;
                    while(head != NULL){
                       pop(&p,&head,&tail);
                    }
                    send_message(3,NULL);
                    if (network_exp == 1){
                         close(accpet_cproxy);
                         change_ip = 1;
                         retransfer = 1;
                         clean_up();
                         break;
                    }
                    recon_telnet = 1;
                    change_ip =1;
                    close(socket_telent);
                    clean_up();
                    break;
                }
                else if (data[0] == 'a')
                {
                    gettimeofday(&time_heartbeat_recv, NULL);
                    char result[4];
                    result[0] = data[1];
                    result[1] = data[2];
                    result[2] = data[3];
                    result[3] = data[4];
                    int get = *((int *)result);

                    node *pop_result = NULL;
                    if (head != NULL)
                    {
                        int delete_time = head->ack;
                        int i;
                        for (i = 0; i <= get - delete_time; i++)
                        {
                            pop(&pop_result,&head,&tail);
                        }
                    }
                    
                }
                else
                {
                    send_message(2, NULL);
                    if (network_exp == 1){
                         close(accpet_cproxy);
                         change_ip = 1;
                         retransfer = 1;
                         clean_up();
                         break;
                    }
                    buff_cproxy[0] = data[5];
                    gettimeofday(&time_heartbeat_recv, NULL);
                    FD_ZERO(&writefds);
                    FD_SET(socket_telent, &writefds);
                    int m = socket_telent + 1;
                    struct timeval tv1;
                    tv1.tv_sec = 10;
                    tv1.tv_usec = 0;
                    rv = select(m, NULL, &writefds, NULL, &tv1);
                    if (rv == -1)
                    {
                        fprintf(stderr, "Select return -1 in 348th\n");
                    }
                    else if (rv == 0)
                    {
                        // do nothing
                    }

                    // send cproxy's message to telent
                    else
                    {
                        if (FD_ISSET(socket_telent, &writefds))
                        {
                            char result[4];
                            result[0] = data[1];
                            result[1] = data[2];
                            result[2] = data[3];
                            result[3] = data[4];
                            int get = *((int *)result);
                            if (get > accept_ackNum)
                            {
                                rv = send_all(socket_telent, buff_cproxy, 1);
                                accept_ackNum = get;
                            }
                    
                            if (rv == -1)
                            {
                                close(accpet_cproxy);
                                change_ip = 1;
                                retransfer = 1;
                                clean_up();
                                break;
                            }
                        }
                    }
                }
            }

            if (FD_ISSET(socket_telent, &readfds))
            {
                if (time_heartbeat_recv.tv_sec != 0)
                {
                    gettimeofday(&time_current, NULL);
                    timersub(&time_current, &time_heartbeat_recv, &time_result);
                    if (time_result.tv_sec >= 3)
                    {
                        close(accpet_cproxy);
                        change_ip = 1;
                        retransfer = 1;
                        clean_up();
                        break;
                    }
                }
                // receive message from telent
                rv = recv_all(socket_telent, buff_telent, 1);
                // When recv( ) returns 0, socket should be closed
                // Then will go back to the listenin
                // reconnect to telnet
                if (rv == 0)
                {
                    // we need to reconnect
                    sp_exit = 1;
                    if (head == NULL && tail == NULL)
                    {
                        close(socket_telent);
                        close(accpet_cproxy);
                        reconnect = 1;
                        retransfer = 0;
                        clean_up();
                        break;
                    }
                }
                else if (rv == -1)
                {
                    close(accpet_cproxy);
                    change_ip = 1;
                    retransfer = 1;
                    clean_up();
                    break;
                }

                else
                {
                    insert(number++, buff_telent,&head,&tail,0);
                }
            }
            // clean buffer
        }
        if (head != NULL && tail != NULL)
        {
            if (count_time >= 1)
            {
                gettimeofday(&time_retransfer_current, NULL);
                timersub(&time_retransfer_current, &time_retransfer_send, &time_result);
                if (head->ack != last_send)
                {
                    send_message(1, head);
                    if (network_exp == 1){
                        close(accpet_cproxy);
                        change_ip = 1;
                        retransfer = 1;
                        clean_up();
                        break;
                    }
                    last_send = head->ack;
                    gettimeofday(&time_retransfer_send, NULL);
                }
                else
                {
                    if (time_result.tv_usec > 700)
                    {
                        send_message(1, head);
                        if (network_exp == 1){
                             close(accpet_cproxy);
                             change_ip = 1;
                             retransfer = 1;
                             clean_up();
                             break;
                        }
                        last_send = head->ack;
                        gettimeofday(&time_retransfer_send, NULL);
                    }
                }
            }
            else
            {
                gettimeofday(&time_retransfer_send, NULL);
                send_message(1, head);
                if (network_exp == 1){
                    close(accpet_cproxy);
                    change_ip = 1;
                    retransfer = 1;
                    clean_up();
                    break;
                }
                last_send = head->ack;
            }
            count_time++;
        }
        clean_up();
    }
}
int main(int argc, const char *argv[])
{

    if (argc < 2)
    {
        fprintf(stderr, "No port input\n");
        exit(1);
    }
    port_cproxy = get_port(argv[1]);
    signal(SIGPIPE, SIG_IGN);
    while (1)
    {
        init();
        time_heartbeat_send = (struct timeval){0};
        time_heartbeat_recv = (struct timeval){0};
        time_current = (struct timeval){0};
        count_time = 0;
        network_exp = 0;
    }
}