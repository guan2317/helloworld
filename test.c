#include <stdio.h> 	//标准输入输出定义
#include <stdlib.h>	//标准函数库定义
#include <unistd.h>	//Unix标准函数定义
#include   <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>	//错误号定义
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>  
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h> //在使用arm-linux-gcc编译的时 需要加上-lphrenad

#define R_PORT 502
#define YES 1
#define BACKLOG 7
int ret = 0;

#define MAXDATASIZE 54
#define MAXNODESIZE 20 //定义最大节点的个数
#define PORT 5222
#define NodeFlag 18  //表示以1开始计数（不是0），数据包的第2个字节为节点的标志信息位，在gatwayserves中使用！
#define REQUEST_Flag 7 //表示以1开始计数的开始位置 第REQUEST_Flag个字节，表示需要返回该节点的数据！


struct nodesdata{
	unsigned char nodedata[34];
};	
struct nodesdata BUF[MAXNODESIZE];



struct argument{
	int fd;
	int port;
	int serverfd;
};
struct argument recvDataFD;


unsigned char nodesID[MAXNODESIZE]={0x01,0x02,0x03};//初始化所有节点的ID为0x00;
int i_nodes=3;							//初始化其中的节点个数为0;

//函数申明
void creatserver(struct argument *p);//创建TCP服务器
void creat_server_sockfd4(int *sockfd, struct sockaddr_in *local, int portnum);//创建SOCKET
void* soureDataPrase(void *arg);
int ReadData(int fd,char *p);
int praseStrToData(unsigned char *str,int length);
int SocketConnected(int sock); 

//函数申明
void gatwayserves(char *argv[]);
void printAllInBUF(const struct nodesdata n_buf[],int n_nodes);
void waitting_toConnect(int seconds);
int compareNode_exist(unsigned char nodes[], unsigned char node);
int getDataFromBUF(int position, unsigned char start, unsigned char regs,unsigned char data[]);

int main(int argc,char *argv[])
{

	pthread_t r_id;
	pthread_t source_Data_id;
	pthread_t gatway_id;
	
	recvDataFD.port = R_PORT;
	
	printf("Test_started waitting...\n");
	sleep(5);
	printf("Test_finished waitting!!!\n");
	
	
	pthread_create(&r_id,NULL,(void *)creatserver,&recvDataFD);
	pthread_create(&source_Data_id,NULL,(void *)soureDataPrase,&recvDataFD);
	pthread_create(&gatway_id,NULL,(void *)gatwayserves,NULL);
	
	while(1) {
					sleep(1);
	}
	return 0;
}





void gatwayserves(char *argv[])
{
	struct sockaddr_in server;
	
	int sockfd, num;
		//struct hostent *he;
	unsigned char commond[9]={0x00,0x00,0x00,0x01,0x01,0x01,0x00,0xa2,0xa1};
		

		
		//创建套接字
		if((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1)
			{
					printf("socket() error!\n");
					exit(1);
			} 
	
		bzero(&server, sizeof(server)); 
		server.sin_family = AF_INET; 
		//server.sin_addr = *((struct in_addr *)he->h_addr);
		server.sin_port = htons(PORT); 
		server.sin_addr.s_addr = inet_addr("192.168.1.10");//写死的服务器端地址
		
		//与服务器尝试建立连接
		while(1)
		{
			if(connect(sockfd, (struct sockaddr *)&server, sizeof(server))==-1)
				{
					printf("Test_Client_connect() error! Trying to connect again after 5 seconds......\n");
						
				} 
			else
				{
					printf("Test_Client:connected to 192.168.1.10:%d",PORT);
					break;
				}
			waitting_toConnect(5);//等待五秒然后重新连接
			
			
		}
		
		
					//1、发送数据
					if(9 == send(sockfd, commond,9, 0))
						{
								printf("send data successfull\n");
						}
					else
						{
								printf("send data  failed!!!\n");
						}
						
						
		while (1) 
			{
					
					sleep(1);
					unsigned char buff[MAXDATASIZE]={0x00};//接收数据的缓冲区
					
					//2、等待接收数据
					int size=0;
					size=recv(sockfd, buff, sizeof(buff), 0);
					
					//打印接收到的数据
					printf("TEST_Client recived a dataPackage, size=%d\n",size);
					
					//buff[size] = '\0';
					//printf("%d",buff[0]);
					
					int j=0;
					for(j=0;j<size;j++)
						{
							printf("%02X ", (unsigned char)buff[j]);
						}
					printf("\n");
					
					//**************************************************************************************//
					//以下部分对数据包进行分析和存储！！！
					//如果要对多个节点的数据包分别存储，需要根据节点的标志信息，对收到的数据包进行解析;		//
					//并将节点的最新数据更新到BUF结构中！！！										    	//
					//**************************************************************************************//
					
					
					
					
					/****************************************************************
					if(0x01==buff[1])
					{
						memcpy(BUF1,buff+16,34);
						for(j=0;j<34;j++)
						
						printf("%x ",BUF1[j]);
						printf("\n");
						
					}else if(0x02==buff[1])
						{
							memcpy(BUF2,buff+16,34);
							for(j=0;j<34;j++)
						
							printf("%x ",BUF2[j]);
							printf("\n");
							
						}
						else 
							printf("NOT EXIST!!!\n");
					********************************************************************/
				
					//1、取出节点的ID取出，暂存在node中
					unsigned char node=buff[NodeFlag-1];//根据节点标识所在的位置信息，取出节点标志node
					
					//2、与已有节点【将节点的URL，用nodesID数组存储】比较，判断是否需要新建一个数组区来存放其数据！！
					if(i_nodes==0)
					{
						//nodes为空，则将数据放入第一个数组区（即BUF[0].buf中）
						
						memcpy(BUF[i_nodes].nodedata,buff+16,34);
						nodesID[i_nodes]=node;	
						i_nodes++;
						
					}else
						{
						
							//nodesID中存在节点的话，将node与其中的所有其他节点比较;
							//存在同一节点则返回其在nodesID中的位置i，不存在则返回-1。
							int k=compareNode_exist(nodesID,node);
							
							if(-1!=k)
							{
								//存在，需要知道是nodesID中的第几个，并在这里处理！！！
								memcpy(BUF[k].nodedata,buff+16,34);
								
							}else{
								
								//不存在，并在这里处理！！！
								
								memcpy(BUF[i_nodes].nodedata,buff+16,34);
								
								nodesID[i_nodes]=node;
								i_nodes++;
								
							}
						
						}
					
					
					//memcpy(BUF+18,buff+16,16);
					
					printAllInBUF(BUF,i_nodes);
			
			}
			
		close(sockfd);
		
		
}

void printAllInBUF(const struct nodesdata n_buf[],int n_nodes)
{
	
	printf("\n***************Data In BUF Structure!******************\n");
					int i,j=0;
					
					for(i=0;i<n_nodes;i++)
					{
						
						printf("BUF[%d]:\n",i);
						for(j=0;j<34;j++)
						{
							printf("%x ;",n_buf[i].nodedata[j]);
						}
						printf("\n");
						
					}
	printf("\n***************Data In BUF Structure!******************\n");
	
}

int compareNode_exist(unsigned char nodes[], unsigned char node)
{
	/***************************************************/
	//
	//参数nodes：用于记录节点ID的数组；
	//参数inodes：该数组中已有的节点个数；
	//参数node:需要查找的节点ID。
	//
	//返回：如果存在则返回对应的位置i，否则返回-1.
	//
	//
	//
	/***************************************************/
	
	int i=0;
	int result=-1;//如果相等返回位置i，否则返回-1
	
	
	for(i=0;i<i_nodes;i++)
	{
		if(node==nodes[i])
		{
			printf("node:%x existed in nodesID[i]=%x\n",node,nodes[i]);
			result=i;
			break;
		}
	}
	return result;
		
}

void waitting_toConnect(int seconds)
{
	
	int i=0;
	
	for(i=seconds;i>0;i--)
	{
		printf("%d\n",i);
		sleep(1);
	}		
			
}



//创建服务器，用于接收客户端的！！！连接！！！请求，为单客户端模式。
void creatserver(struct argument *p)
{
	char addrstr[100];
	int serverfd;
	struct sockaddr_in local_addr_s;
	struct sockaddr_in from;
	unsigned int len = sizeof(from);

	creat_server_sockfd4(&serverfd,&local_addr_s,p->port);

	while(1)
	{
		p->fd = accept(serverfd, (struct sockaddr*)&from, &len);
		if(ret == -1){
			perror("accept");
			exit(EXIT_FAILURE);
		}
		struct timeval time;
		gettimeofday(&time, NULL);
		printf("time:%lds, %ldus\n",time.tv_sec,time.tv_usec);
		printf("a IPv4 client from:%s\n",inet_ntop(AF_INET, &(from.sin_addr), addrstr, INET_ADDRSTRLEN));
	}

}

void creat_server_sockfd4(int *sockfd, struct sockaddr_in *local, int portnum)
{
	int err;
	int optval = YES;
	int nodelay = YES;

	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(*sockfd < 0){
		perror("socket");
		exit(EXIT_FAILURE);
	}
	err = setsockopt(*sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
	if(err){
		perror("setsockopt");
	}
	err = setsockopt(*sockfd,IPPROTO_TCP,TCP_NODELAY,&nodelay,sizeof(nodelay));
	if(err){
		perror("setsockopt");
	}


	memset(local, 0, sizeof(struct sockaddr_in));
	local->sin_family = AF_INET;
	local->sin_addr.s_addr = htonl(INADDR_ANY);
	local->sin_port = htons(portnum);

	err = bind(*sockfd, (struct sockaddr*)local, sizeof(struct sockaddr_in));
	if(err < 0){
		perror("bind");
		exit(EXIT_FAILURE);
	}
	err = listen(*sockfd, BACKLOG);
	if(err < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}

}

void* soureDataPrase(void *arg)
{
	fd_set readfd;
	int ret = 0;
	int ret_send = 0;
	int maxfd = 0;
	int num = 0;
	char *p;
	int i=0;
	
	printf("modbus_tcp adapter start, reviece port is:%d\n",R_PORT);
	while(1){
		maxfd = 0;
		struct timeval timeout;
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		
		if(recvDataFD.fd != 0){
			
			FD_ZERO(&readfd);
			FD_SET(recvDataFD.fd, &readfd);
			maxfd = recvDataFD.fd + 1;			
			ret = select(maxfd, &readfd, NULL, NULL, &timeout);
			
			if(ret == -1){
				perror("select");
			}else 
				if(ret > 0 && FD_ISSET(recvDataFD.fd, &readfd)){ 
				//printf("ret = %d\n",ret);
				p = (char *)malloc(sizeof(char) * 1000);
				num = ReadData(recvDataFD.fd, p);//读取请求
				
				if(num > 0) {
					printf("receive %d byte\n",num);
					printf("received modbus_tcp data is:");
					for(i=0;i<num;i++)
					{
						printf("%x ",p[i]);
					}
					printf("\n");
					praseStrToData(p,num);//解析请求并发送数据

				} 
				else if (num == -1) {
					//client disconnect
					FD_CLR(recvDataFD.fd, &readfd);
					printf("Client disconnect!\n");				
					//close(recvDataFD.fd);  //关闭socket连接   
					recvDataFD.fd=0;		//将连接描述符复位
				}
				free(p);
			}
		}
	}
}

int ReadData(int fd,char *p)
{
	/***********************************************/
	//
	//功能：用于读取PC客户端发送过来的请求
	//
	//参数：fd 连接对象的套接字描述符
	//		p 读取到的数据存入P中
	//
	//返回：rcv出错，返回0;
	//		网络中断，返回-1；
	//		成功读取则返回读取到的字节数num，读取到12个字节就结束读取。
	//
	/***********************************************/
	
	char c;
	int ret = 0;
	int num = 0;

	ret = recv(fd, &c, 1,0);
	if(ret < 0){
		perror("recv");
		return 0;
	}
	else if(ret == 0){
		printf("无数据接收\n");
		
		return -1;
	}
	else{
			*p = c;
			num = 1;
			while(1){
				ret = recv(fd, p + num, 1, 0);
					num++;
				if(num == 12)
					return num;
			}
	}	
}

int praseStrToData(unsigned char *str,int length)
{
	/*********************************************************/
	//解析PC客户端端发过来的请求数据str，并根据其请求类型
	//将相应的数据包回应给客户端！！！
	//
	//
	//
	/*********************************************************/
	
	
	int i;
	int ret_send = 0; 
	int datalen = 0;
	
	unsigned char startAddress=0x00; //读取寄存器的起始地址
	unsigned char regs=0x00;//读取寄存器的数量
	
	unsigned char nodeNumofReq;//请求节点ID
	
	if(str == NULL) {
		return -1;
	}

	if(length < 0 && length >100)
		return -1;
	unsigned char NewData[80]={0};
	unsigned char ErrorData[9]={0};

	memcpy(NewData,str,4);
	NewData[4]= 0x00;
	
	NewData[5]= 0x25;	//接下来的数据长度【不包括本身】
	
	
	NewData[6]= str[6]; //从站地址
	NewData[7]= str[7]; //功能码
	
	//srt[8];
	startAddress=str[9];//起始地址；
	regs=str[11];//寄存器个数；
	
	NewData[8]= ((str[10]<<8)+str[11]);
	
	nodeNumofReq=str[REQUEST_Flag-1];
	
	printf("nodeNumofReq is: %02x \n",nodeNumofReq);
	
/*	if(0x01==str[6])
	{
		memcpy(NewData+9,BUF1,34);
	}else if(0x02==str[6])
		{
			memcpy(NewData+9,BUF2,34);
		}
		else
			printf("\nthe node requested NOT EXIST!!\n");

*/	
	
	//判断该节点是否存在于BUF中，通过记录节点ID的i_nodes数组来判断
	int position=-1;
	if((position=compareNode_exist(nodesID,nodeNumofReq))!=-1)
		{
			//1，存在，找到节点位置
			//并将将该节点的数据赋给NewData
			printf(" REQ position is  : %d !!!\n",position);
			
			unsigned char data[34]={0x00};
			
			int lenth=getDataFromBUF(position, startAddress, regs, data);
			
			printf("the length of the getting data data from BUF is :%d\n",lenth);
			
			
			memcpy(NewData+9,data,lenth);
			
			printf("Return data is :\n");
			for(i=0;i<lenth;i++)
			{
				printf("%x \n",data[i]);
			}
				
			NewData[8]=lenth;//数据字段的长度
			NewData[5]=3+lenth;
		}
		else{
			
			printf("the data of nodeNumber:%02x that PC client requested doesnot exist!\n",nodeNumofReq);
			//2，不存在
			
		}
	
	datalen=9+((str[10]<<8)+str[11])*2;//发送的总字节个数
	
	memcpy(ErrorData,str,4);
	ErrorData[4]= 0x00;
	ErrorData[5]= 0x03;
	ErrorData[6]= str[6];		//设备ID
	ErrorData[7]= str[7]+0x80;  //功能码，出错的情况下由原来的功能码+0x80
	ErrorData[8]= 0x01;
	

	
	if(SocketConnected(recvDataFD.fd)) 
	{
		
		if(  (-1!=position)  &&  (startAddress+regs)<18   )	
		//if(str[7]==0x03  && NewData[8]==34 )	
		{
			
			ret_send = send(recvDataFD.fd,NewData,datalen, 0);						
			if(ret_send < 0) {//错误检查
				perror("send");
				close(recvDataFD.fd);
			}
			else if(ret_send == 0)//错误检查
				printf("连接断开\n");
			else
				printf("write recvDataFD_fd! ret_send=%d\n",ret_send);
				
			printf("send modbus_tcp normal data is:");
					for(i=0;i<ret_send;i++)
					{
						printf("%x ",NewData[i]);
					}
					printf("\n");
		}
		else
			{
				ret_send = send(recvDataFD.fd,ErrorData,9, 0);						
			if(ret_send < 0) {
				perror("send");
				close(recvDataFD.fd);
			}
			else if(ret_send == 0)
				printf("连接断开\n");
				else
					printf("write recvDataFD_fd! ret_send=%d\n",ret_send);
					printf("send modbus_tcp Errordata is:");
						for(i=0;i<ret_send;i++)
						{
							printf("%x ",ErrorData[i]);
						}
						printf("\n");
				
			}
	}
	
	return 1;
}

int SocketConnected(int sock) 
{ 
	if(sock<=0) 
		return 0; 
	struct tcp_info info; 
	int len=sizeof(info); 
	getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len); //检测sock的TCP_INFO状态
	if((info.tcpi_state==1))	
	{
		return 1; 
	} 
	else 	
	{  
		return 0; 
	}
}


int getDataFromBUF(int position, unsigned char startAddress, unsigned char regsNum,unsigned char data[])
{
	/******************************************
	//参数1：position————需要获取的设备数据在BUF中的位置；
	//参数2：start————起始位置,这里只用了一个字节来表示；
	//参数3：regs————寄存器数量（乘以2得到需要返回的数据量）,一个寄存器表示读取出两个字节！！！；
	//参数4：data[]————按以上三个参数将读取到的数据存入data[]中；
	//
	//返回：正常则返回取出的字节数，否则返回-1。
	//
	//应该在次函数
	*******************************************/
	
	int lenth=0;//初始化读取到的字节数为0，表示未读取任何字节。
	int i=0;
	
	int start=startAddress;
	int regs=regsNum;
	
	
	if( (start+regs)<=17 && start>=0 && regs>0 ) //输入参数检测
	{
		
		for(i=start*2;i<((start+regs)*2);i++)
		{
			data[lenth]=BUF[position].nodedata[i];
			lenth++;
		}
		
		
		printf("getDataFromBUF:\n");
		for(i=0;i<lenth;i++)
		{
			printf("%x \n",data[i]);
		}
		
		//memcpy(data,BUF[position].nodedata,34);
	}else{
		printf("getDataFromBUF ERROR!!! Input ERROR!!!\n");
	}
	
	return lenth;
	
	
}
