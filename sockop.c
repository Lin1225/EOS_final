/*
sockop.c
*/
 #include "sockop.h"

/*
* passivesock - allocate & bind a server socket using TCP or UDP
*
* Arguments:
* service - service associated with the desired port
* transport - transport protocol to use ("tcp" or "udp")
* qlen - maximum server request queue length
*/
int passivesock( const char *service , const char *transport){
	int sockfd, type;
	struct sockaddr_in serverInfo;

	/* Use protocol to choose a socket type */
	if(strcmp(transport,"udp")==0)
		type = SOCK_DGRAM;
	else 
		type = SOCK_STREAM;
	/* Allocate a socket */
	sockfd = socket(AF_INET,type,0);
	if(sockfd < 0){
		errexit("Can’t create socket : %s \n" , strerror(errno));
		return 0;
	}

	bzero(&serverInfo, sizeof(serverInfo));
	serverInfo.sin_family = PF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;
	serverInfo.sin_port = htons(atoi(service));

	int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	/* Bind the socket */
	if(bind(sockfd,( struct sockaddr *)&serverInfo, sizeof(serverInfo))< 0)
		errexit("Can’t bind to port : %s:%s \n" ,service,strerror(errno));

	return sockfd;
}
/*
* connectsock - allocate & connect a socket using TCP or UDP
*
* Arguments:
* host - name of host to which connection is desired
* service - service associated with the desired port
* transport - name of transport protocol to use ("tcp" or "udp")
*/
int connectsock (const char *host,const char *service , const char *transport){
	int sockfd, type;
	struct sockaddr_in info;

	/* Use protocol to choose a socket type */
	if(strcmp(transport,"udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;
	/* Allocate a socket */
	sockfd = socket(AF_INET, type, 0);
	if(sockfd < 0)
		errexit("Can’t create socket : %s \n" , strerror(errno));

	bzero(&info, sizeof(info));
	info.sin_family = PF_INET;
	info.sin_addr.s_addr = inet_addr(host);
	info.sin_port = htons(atoi(service));

	if(connect(sockfd,(struct sockaddr *)&info,sizeof(info)) == -1)
		errexit("Can’t connect to  %s.%s:%s \n" ,host,service,strerror(errno));

	return sockfd;
}
