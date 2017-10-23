#include <stdio.h>
#include <stdlib.h>
#include "smbhdr.h"
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib,"ws2_32.lib")

SOCKET client_connect(char* server,unsigned short port);
SOCKET create_serv(unsigned short port,int cnum);
int showmem(byte *sc,int ptleng);
int Send(SOCKET s,char *buf,int len);
int Recv(SOCKET s,char *buf,int len);
unsigned int c_global_id = 0, s_global_id = 0;
typedef unsigned long long int ullong;

int main(int argc,char **argv)
{
    WSADATA wsaData;
    BYTE buf[1024];
    char file_m[1024],*p;
    int i,addrsize,a, b,d,seg;
    unsigned char *pbin_buf;
    unsigned int bin_size;
    
    if(WSAStartup(MAKEWORD(2,2),&wsaData ))
    {
        printf("[-] WSAStartup failed:%d\n",GetLastError());
        return 0;
    }
    if(argc < 3) {
        printf("Usage:%s <binary> <txtfile>\n", argv[0]);
        return 0;
    }

    srand((unsigned)time(NULL));  
    if(!read_into_buffer(argv[1], &pbin_buf, &bin_size)) {
        return 0;
    }
    dump_into_txt(argv[2], pbin_buf, bin_size); 
    free(pbin_buf);
    _snprintf(file_m, sizeof(file_m), "%s", argv[2]);
    p = strchr(file_m, '.');   
    if(p) *p = 0;
    _snprintf(buf, sizeof(buf), "eva -cf %s -sf %s %s.pcap -si 10.43.19.49 -di 10.43.19.26 -sp %u -dp 445", argv[2], argv[2], file_m, rand() & 0xffff);
    printf("[CMD] %s\n", buf);
    system(buf);
    WSACleanup();
}          
typedef struct _req_s{
    ullong msgid;
    ullong offset;
    unsigned int size;
    unsigned char *reqbuf;
    unsigned int reqlen;
    unsigned char *rspbuf;
    unsigned int rsplen;
}REQ_S, *PREQ_S;
unsigned char file_req_temp[] = "\x00\x00\x00\x71\xfe\x53\x4d\x42\x40\x00\x01\x00\x00\x00\x00\x00\x08\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x16\x00\x00\x00\x00\x00\x00\x00\xff\xfe\x00\x00\x01\x00\x00\x00\x39\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x31\x00\x50\x00\x88\x63\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\xf5\x00\x00\x00\x70\x00\x00\x00\x0d\x00\x00\x00\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

unsigned char file_rsp_temp[] = "\x00\x01\x00\x50\xfe\x53\x4d\x42\x40\x00\x01\x00\x00\x00\x00\x00\x08\x00\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00\x14\x00\x00\x00\x00\x00\x00\x00\xff\xfe\x00\x00\x01\x00\x00\x00\x39\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x11\x00\x50\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00";

int dump_into_txt(char *df, unsigned char *pbin, unsigned int binlen) {
    FILE *fp;
    int data_size = 1304, left_size = 0;
    int rand_times, i,a, b,c;
    rand_times = (binlen / data_size);
    REQ_S *psmb_file = NULL, *p;
    ullong msg_start = 120;
    int *pids = NULL;   
 
    left_size = binlen % data_size;
    if(left_size) rand_times++;
    
    fp = fopen(df, "wb");
    if(!fp) {
        printf("fopen write failed.\n");
        return 0;
    }
    printf("dump header packets to file..\n");
    makehdr(fp, binlen);
    psmb_file = (PREQ_S) malloc(rand_times * sizeof(REQ_S));
    memset(psmb_file, 0, rand_times * sizeof(REQ_S));
    printf("need req/rsp %u teims..\n", rand_times); 
    for(i = 0; i < rand_times; i++) {
        p = &psmb_file[i];
        p->msgid = i + msg_start;
        p->offset = i * data_size;
        p->size = data_size;
        if(i == (rand_times - 1)) p->size = left_size;
        p->reqbuf = (unsigned char *) malloc(sizeof(file_req_temp) - 1);
        memcpy(p->reqbuf, file_req_temp, sizeof(file_req_temp) - 1);
        *(ullong*)(p->reqbuf + 28) = p->msgid;
        *(unsigned int*)(p->reqbuf + 72) = p->size;
        *(ullong*)(p->reqbuf + 76) = p->offset;
        p->reqlen = sizeof(file_req_temp) - 1;

        p->rsplen = sizeof(file_rsp_temp) + p->size - 1;
        p->rspbuf = (unsigned char *) malloc(p->rsplen);
        memcpy(p->rspbuf, file_rsp_temp, sizeof(file_rsp_temp) - 1);
        *(ullong*)(p->rspbuf + 28) = p->msgid;
        *(unsigned int*)(p->rspbuf + 72) = p->size;
        //*(ullong*)(p->rspbuf + 76) = p->offset;
        p->rspbuf[1] = 0;
        *(unsigned short *) (p->rspbuf + 2) = ntohs((p->size + 80) & 0xffff);
        memcpy(p->rspbuf + sizeof(file_rsp_temp) - 1, pbin + i * data_size, p->size);
    }
    pids = (int *) malloc(rand_times * sizeof(int));
    //req
    for(i = 0; i < rand_times; i++) {
        pids[i] = i;
    }
    a = rand_times;
    printf("randomize req packets...\n");
    while(a > 0) {
        b = rand() % a; 
        printf(" %d", pids[b]);
        write_mem(fp, psmb_file[pids[b]].reqbuf, psmb_file[pids[b]].reqlen, 0); 
        for(c = b + 1; c < rand_times; c++) {
            pids[c - 1] = pids[c];
        }
        a--;
    }
    printf("\n");
    //write_mem(fp, psmb_file[0].reqbuf, psmb_file[0].reqlen, 0); 
    //rsp
    for(i = 0; i < rand_times; i++) {
        pids[i] = i;
    }
    a = rand_times;
    printf("randomize rsp packets...\n");
    while(a > 0) {
        b = rand() % a; 
        printf(" %d", pids[b]);
        write_mem(fp, psmb_file[pids[b]].rspbuf, psmb_file[pids[b]].rsplen, 1); 
        for(c = b + 1; c < rand_times; c++) {
            pids[c - 1] = pids[c];
        }
        a--;
    }
    printf("\n");
    //write_mem(fp, psmb_file[0].rspbuf, psmb_file[0].rsplen, 1); 
    printf("dump foot packets to file..\n");
    makefoot(fp); 
DONE:
    fclose(fp);
    if(pids) free(pids);
    if(psmb_file) 
    {
        for(i = 0; i < rand_times; i++) {
            free(psmb_file[i].reqbuf);
            free(psmb_file[i].rspbuf);
        }
        free(psmb_file);
    }
    return 1;
}
int read_into_buffer(char *f, unsigned char **pout, unsigned int *poutlen) {
    FILE *fp;
    unsigned char *p;
    unsigned int plen, file_size;

    fp = fopen(f,"rb");
    if(!fp) {
        printf("fopen failed.\n");
        return 0;
    }
    fseek(fp,0,SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
    
    p = (unsigned char *) malloc(file_size);
    if(!p) {
        printf("malloc failed.\n");
        fclose(fp);
        return 0;
    }
    plen = fread(p, 1, file_size, fp);
    if(plen != file_size) {
        printf("fread error.\n");
        fclose(fp);
        free(p);
        return 0;
    }
    printf("read %u bytes from %s\n", plen, f);
    *pout = p;
    *poutlen = plen;
    return 1;
}
int write_mem(FILE *fp, unsigned char *p, unsigned int len, int direction) {
    char var_name[62];
    unsigned int i;
    memset(var_name, 0, sizeof(var_name));
    if(direction) {
        fprintf(fp, "char peer%d_%u[] = {\r\n", direction, s_global_id);
        s_global_id++;
    }
    else {
        fprintf(fp, "char peer%d_%u[] = {\r\n", direction, c_global_id);
        c_global_id++;
    }
    for(i = 0; i < len; i++) {
        if(i && ((i % 8) == 0) ) {
            fprintf(fp, "\r\n");
        }
        if(i < (len - 1)) fprintf(fp, "0x%.2x, ", p[i]);
        else fprintf(fp, "0x%.2x", p[i]);
    }    
    fprintf(fp, " };\r\n");
    return 1;
}
int makefoot(FILE *fp) {

    write_mem(fp, peer0_40, sizeof(peer0_40), 0);
    write_mem(fp, peer1_313, sizeof(peer1_313), 1);
    write_mem(fp, peer0_41, sizeof(peer0_41), 0);
    write_mem(fp, peer1_314, sizeof(peer1_314), 1);
    write_mem(fp, peer0_42, sizeof(peer0_42), 0);
    write_mem(fp, peer1_315, sizeof(peer1_315), 1);
    write_mem(fp, peer0_43, sizeof(peer0_43), 0);
    write_mem(fp, peer1_316, sizeof(peer1_316), 1);
    write_mem(fp, peer0_44, sizeof(peer0_44), 0);
    return 1;
}
int makehdr(FILE *fp, unsigned int fsize) {
    unsigned int cluster_size;
    ullong a,b;

    cluster_size = (((fsize - 1)/4096) + 1) * 4096;
    a = fsize; 
    b = cluster_size;
    //write_mem(fp, peer0_0, sizeof(peer0_0), 0);
    //write_mem(fp, peer1_0, sizeof(peer1_0), 1);
    //write_mem(fp, peer0_1, sizeof(peer0_1), 0);
    write_mem(fp, peer0_2, sizeof(peer0_2), 0);
    //write_mem(fp, peer1_1, sizeof(peer1_1), 1);
    write_mem(fp, peer1_2, sizeof(peer1_2), 1);
    write_mem(fp, peer0_3, sizeof(peer0_3), 0);
    write_mem(fp, peer1_3, sizeof(peer1_3), 1);
    write_mem(fp, peer0_4, sizeof(peer0_4), 0);
    write_mem(fp, peer1_4, sizeof(peer1_4), 1);
    write_mem(fp, peer0_5, sizeof(peer0_5), 0);
    write_mem(fp, peer1_5, sizeof(peer1_5), 1);
    write_mem(fp, peer0_6, sizeof(peer0_6), 0);
    write_mem(fp, peer1_6, sizeof(peer1_6), 1);
    write_mem(fp, peer1_7, sizeof(peer1_7), 1);
    write_mem(fp, peer0_7, sizeof(peer0_7), 0);
    write_mem(fp, peer1_8, sizeof(peer1_8), 1);
    write_mem(fp, peer0_8, sizeof(peer0_8), 0);
    write_mem(fp, peer1_9, sizeof(peer1_9), 1);
    write_mem(fp, peer1_10, sizeof(peer1_10), 1);
    write_mem(fp, peer0_9, sizeof(peer0_9), 0);
    write_mem(fp, peer1_11, sizeof(peer1_11), 1);
    write_mem(fp, peer0_10, sizeof(peer0_10), 0);
    //file size modify
    *(ullong *)(peer1_12 + 108) = b;
    *(ullong *)(peer1_12 + 116) = a;
    //showmem(peer1_12, sizeof(peer1_12));
    write_mem(fp, peer1_12, sizeof(peer1_12), 1);
    write_mem(fp, peer0_11, sizeof(peer0_11), 0);
    write_mem(fp, peer1_13, sizeof(peer1_13), 1);
    write_mem(fp, peer0_12, sizeof(peer0_12), 0);
    write_mem(fp, peer1_14, sizeof(peer1_14), 1);
    return 1;
}
SOCKET client_connect(char* server,unsigned short port)
{
    struct sockaddr_in cliaddr;
    struct hostent *host;
    SOCKET sockfd;
    
    sockfd=socket(2,1,0);
    if(sockfd == INVALID_SOCKET)
    {
        printf("[-] Socket error:%d",GetLastError());
        return 0;
    }

    if((host = gethostbyname(server)) == NULL)
    {
        printf("[-] Gethostbyname error:%d",GetLastError());
        closesocket(sockfd);
        return 0;
    }

    memset((void*)&cliaddr,0,sizeof(struct sockaddr));
    cliaddr.sin_family=AF_INET;
    cliaddr.sin_port=htons(port);
    cliaddr.sin_addr=*((struct in_addr *)host->h_addr);
    if(connect(sockfd,(struct sockaddr *)&cliaddr,sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
        printf("[-] Connecting failed:%d",GetLastError());
        closesocket(sockfd);
        return 0;
    }
    return sockfd;
}

int Recv(SOCKET s,char *buf,int len)
{
    int i;
    
    i = recv(s,buf,len,0);
    if(i == 0)      printf("[-] Target close socket\n");
    if(i < 0)
    {
        printf("[-] recv error:%d",GetLastError());
    }
    return i;
}

int Send(SOCKET s,char *buf,int len)
{
    int i;
    
    printf("send %d bytes\n", len);
    i = send(s,buf,len,0);
    if(i == 0)      printf("[-] Target close socket\n");
    if(i < 0) 
    {
        printf("[-] send error:%d",GetLastError());
    }
    return i;
}
SOCKET create_serv(unsigned short port,int cnum)
{
    struct sockaddr_in srvaddr;
    int on=1;
    SOCKET sockfd;
    
    sockfd=socket(2,1,0);
    if(sockfd == INVALID_SOCKET)
    {
        printf("[-] socket error:%d",GetLastError());
        return 0;
    }
    
    memset((void*)&srvaddr,0,(size_t)sizeof(struct sockaddr));
    srvaddr.sin_port=htons((unsigned short)port);
    srvaddr.sin_family=AF_INET;
    srvaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char *)&on,sizeof(int));  //so I can rebind the port
    if(bind(sockfd,(struct sockaddr *)&srvaddr,sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
        printf("[-] Binding error:%d",GetLastError());
        closesocket(sockfd);
        return 0;
    }
    if(listen(sockfd,cnum) == SOCKET_ERROR)
    {
        printf("[-] Listening error:%d",GetLastError());
        closesocket(sockfd);
        return 0;
    }
    return sockfd;
}
int showmem(byte *sc,int ptleng)
{
        int i=0,t,a,next=16;

        while(i<ptleng)
        {
                if(i%16==0)
                {
                        printf("\r\n%.8x: ",i);
                }
               
               next=((i+16)>ptleng)?ptleng:(i+16);
               t=i;
               for(;i<next;i++)  printf("%.2x ",sc[i] & 0xff);
               a=next-t;
               for(;a<16;a++)  printf("   ");
               printf(" ");
               while(t<next)
               {
                    if((*(sc+t)<0x1f) || (*(sc+t)>0x7e))
                        printf(".");
                    else
                        printf("%c",sc[t]);
                    t++;
                }
      }
      printf("\r\n");
      return i;

}
