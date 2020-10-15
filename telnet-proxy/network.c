#include <stdio.h>
#include<stdlib.h>
#include "network.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
int get_port(const char *p)
{
    int i;
    int rc = 0;
    for (i = 0; p[i] != '\0'; i++)
    {
        if (p[i] >= '0' && p[i] <= '9')
        {
            rc = rc * 10 + (p[i] - '0');
        }
        else
        {
            fprintf(stderr, "Invalid port component\n");
            exit(1);
        }
    }
    return rc;
} 

int recv_all(int socket_id, char *buff, int size)
{
    int rc = recv(socket_id, buff, size, 0);
    if (rc == -1)
    {
        fprintf(stderr, "Recv error\n");
        return -1;
    }
    if (rc == 0)
    {
        return 0;
    }
    if (rc < size)
    {
        int dis = size - rc;
        while (dis != 0)
        {
            buff += rc;
            rc = recv(socket_id, buff, dis, 0);
            if (rc == -1)
            {
                fprintf(stderr, "Recv error\n");
                return -1;
            }
            dis -= rc;
        }
    }
    return 1;
}
int send_all(int socket_id, char *mes, int size)
{
    int rc = send(socket_id, mes, size, MSG_NOSIGNAL);
    if (rc == -1)
    {
        fprintf(stderr, "Send error.\n");
        return -1;
    }
    if (rc < size)
    {
        int dis = size - rc;
        while (dis != 0)
        {
            mes += rc;
            rc = send(socket_id, mes, dis, MSG_NOSIGNAL);
            if (rc == -1)
            {
                fprintf(stderr, "Send error.\n");
                return -1;
            }
            dis -= rc;
        }
    }
    return 1;
}
void insert(int ack, char *data, node** head, node** tail, int r)
{
 
    node *n = malloc(sizeof(node));
    n->ack = ack;
    *(n->a) = data[0];
    n->next = NULL;
    n->reset = r;
    if (*head == NULL && *tail == NULL)
    {
        *head = n;
        *tail = n;
    }
    if (*head == NULL && *tail != NULL)
    {
        fprintf(stderr, "Error occur 0\n");
        exit(1);
    }
    if (*head != NULL && *tail == NULL)
    {
        fprintf(stderr, "Error occur 1\n");
        exit(1);
    }
    if (*head != NULL && *tail != NULL)
    {
        (*tail)->next = n;
        *tail = n;
    }
}
void pop(node **p, node** head, node** tail)
{

    if (*head == NULL && *tail == NULL)
    {
        fprintf(stderr, "Error occur 2\n");
        exit(1);
    }
    if (*head == NULL && *tail != NULL)
    {
        fprintf(stderr, "Error occur 3\n");
        exit(1);
    }
    if (*head != NULL && *tail == NULL)
    {
        fprintf(stderr, "Error occur 4\n");
        exit(1);
    }
    if (*head != NULL && *tail != NULL)
    {
        if (*head == *tail)
        {
            *p = *head;
            *head = NULL;
            *tail = NULL;
        }
        else
        {

            *p = *head;
            *head = (*head)->next;
        }
    }
}