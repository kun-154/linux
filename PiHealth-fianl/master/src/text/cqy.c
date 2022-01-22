#include "common.h"


int socket_create(int port) {
	int sockfd;
	struct sockaddr_in sock_addr = {0};
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket_create ");
		return -1;
	}

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);


	if (bind(sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(sockfd, 10) < 0) {
		perror("listen");
		return -1;
	}

	return sockfd;
}



int sock_connect(int port, char *host) {
	int sockfd;
	struct sockaddr_in dest_addr = {0};
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket_create ");
		return -1;
	}

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = inet_addr(host);

	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
		perror("connect");
		return -1;
	}
	return sockfd;
}