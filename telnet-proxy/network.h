#ifndef network_h
#define network_h


typedef struct node
{
    char a[1];
    int ack;
    struct node *next;
    int reset ;
} node;
void insert(int ack, char *data, node** head, node** tail, int r);
int recv_all(int socket_id, char *buff, int size);
int get_port(const char *p);
int send_all(int socket_id, char *mes, int size);
void pop(node **p,node** head, node** tail);
#endif 