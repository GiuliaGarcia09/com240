/*
  gcc -o exe ./trabalho_02/clientes/cliente_b.c -lm && ./exe
  possui os arquivos requisitados e os envia
*/

/* incluindo as bibliotecas */
#include<stdio.h>
#include<string.h>
#include<locale.h>
#include<winsock2.h>
#include<sys/types.h>
#define tam_buffer 1024
#define porta_servidor 10222
#define porta_local 10333

/* criando função de soma de verificação */
int checksum(char *buffer,int tam_msg);

/* função principal */
int main(){
  setlocale(LC_ALL, "Portuguese");

  WSADATA data;
  SOCKET sock_cliente;

  /* criando variaveis e structs dos endereços */
  int addr_tam = sizeof(SOCKADDR), rec_msg_tam, status, pkt_cont, msg_tam,er;
  char *buffer, nome_arquivo[50], ch_ree, *buffer_recv;
  struct sockaddr_in addr_cliente1, addr_cliente;
  
  buffer = (char*)malloc(tam_buffer);
  buffer_recv = (char*)malloc(tam_buffer);
  FILE *arquivo;

  /* iniciando sockets no windows */
  if (WSAStartup(MAKEWORD(2, 2), &data)!=0){
    printf("Falha WSAStartup\n");
    return (-1);
  }

  /* criando socket local */
  sock_cliente=socket(AF_INET,SOCK_DGRAM,0);
  if (sock_cliente<0){
    printf("Erro ao iniciar socket.\n");
    return (-1);
  }

  /* dados do socket do cliente */
  memset(&addr_cliente, 0, sizeof(addr_cliente));
  addr_cliente.sin_family = AF_INET;
  addr_cliente.sin_port = htons(porta_local);
  addr_cliente.sin_addr.s_addr = inet_addr("127.0.0.1");
  
  /* bind no socket */
  if (bind(sock_cliente,(struct sockaddr *)&addr_cliente , sizeof(addr_cliente))<0){
    printf ("Erro bind() : %i\n",er=WSAGetLastError());
    return (-1);
  }

  /* aguardando conexões */
  listen(sock_cliente,1);
  printf ("Aguardando contato de cliente...\n");
  ch_ree = '1';

  while (1){
    memset(buffer,'\0', tam_buffer);

    /* captura erro */
    if ((rec_msg_tam=recvfrom(sock_cliente, buffer, tam_buffer, 0, (struct sockaddr *) &addr_cliente1, &addr_tam)) == SOCKET_ERROR){
      printf("recvfrom() failed with error code : %d\n" , WSAGetLastError());
      exit(EXIT_FAILURE);
    }

    if (buffer[0]!='\0'){
      strcpy(nome_arquivo,buffer);
      arquivo=fopen(nome_arquivo,"rb");

      if (arquivo==NULL){
        /* captura erro */
        strcpy(buffer,"0Erro-0002");
        if (sendto(sock_cliente, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente1, sizeof(addr_cliente1)) == SOCKET_ERROR){
          printf("sendto() failed with error code : %d\n" , WSAGetLastError());
          exit(EXIT_FAILURE);
        }
      } else {
        /* captura erro */
        strcpy(buffer,"5");
        if (sendto(sock_cliente, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente1, sizeof(addr_cliente1)) == SOCKET_ERROR){
          printf("sendto() failed with error code : %d\n" , WSAGetLastError());
          exit(EXIT_FAILURE);
        }
        printf("Enviando arquivo %s\n",nome_arquivo);
        pkt_cont=0;
        status=0;

        while(1){
          memset(buffer,'\0', tam_buffer);
          msg_tam=fread(&buffer[4],1,tam_buffer-4,arquivo);

          if (msg_tam==tam_buffer-4){
            buffer[0]='1';
            checksum(buffer,msg_tam);
            status=1;
          }else{
            buffer[0]='0';
            checksum(buffer,msg_tam);
            status=0;
          }

          if (ch_ree=='1'){
            ch_ree='0';
          }else{
            ch_ree='1';
          }

          buffer[1]=ch_ree;

          if (sendto(sock_cliente, buffer, msg_tam+4 , 0 , (struct sockaddr *) &addr_cliente1, sizeof(addr_cliente1)) == SOCKET_ERROR){
            printf("sendto() failed with error code : %d\n" , WSAGetLastError());
            exit(EXIT_FAILURE);
          }

          pkt_cont++;

          /* aguardando ack */
          while(1){
            memset(buffer_recv,'\0', tam_buffer);
            
            printf("Aguardando ack...\n");

            /* captura erro */
            if ((rec_msg_tam=recvfrom(sock_cliente, buffer_recv, tam_buffer, 0, (struct sockaddr *) &addr_cliente1, &addr_tam)) == SOCKET_ERROR){
              printf("recvfrom() failed with error code : %d\n", WSAGetLastError());
              exit(EXIT_FAILURE);
            }

            if(buffer[0]!='\0'){
              printf("Buffer = %s\n", buffer_recv);

              if(strcmp(buffer_recv, "ack1")==0){
                printf("Ack %d OK\n", pkt_cont);
                break;

              }else if(strcmp(buffer_recv,"ack0")==0){
                printf("Ack %d com erro\nReenviar pacote", pkt_cont);
                if (sendto(sock_cliente, buffer, msg_tam+4 , 0 , (struct sockaddr *) &addr_cliente1, sizeof(addr_cliente1)) == SOCKET_ERROR){
                    printf("sendto() failed with error code : %d\n", WSAGetLastError());
                    exit(EXIT_FAILURE);
                }
                system("pause");
              }
            }
          }
          if(status==0){
            break;
          }
        }
      }
      printf("Envio concluido com sucesso\n");
      fclose(arquivo);
    }
  }
}

/* implementação do checksum */
int checksum (char *buffer,int tam_msg){
  unsigned int sum=0,i;
  char ch[3];

  for (i=4;i<tam_msg+4;i++){
    sum+=buffer[i];
  }

  sum=sum%16;
  if (sum<10){
    sum=sum*10;
  }
  
  itoa(sum,ch,10);
  buffer[2]=ch[0];
  buffer[3]=ch[1];
  return (0);
}