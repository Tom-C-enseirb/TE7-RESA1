#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"

#define SIZE_TAB 128


int first_fds_available(struct pollfd *fds, int size) {
	for (int i = 1; i < size; i++) {
		if (fds[i].fd == -1) {
			return i;
		}
	}
	return -1; 
}

int echo_server(int sockfd) {
	char buff[MSG_LEN];
	while (1) {
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Receiving message
		if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
			return -1;
		}
		printf("Received from fd %d : %s", sockfd, buff);
		// Sending message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			return -1;
		}
		printf("Message sent!\n");
		return 0;
	}
}

int handle_bind(const char *server_port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, server_port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char** argv) {
	struct sockaddr cli;
	int sfd, connfd, i;
	socklen_t len;
	
	if (argc != 2) {
		fprintf(stderr, "pas le bon nombre d'arguments \n");
		exit(EXIT_FAILURE);
	}
	
		sfd = handle_bind(argv[1]);
	if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}
	

	struct pollfd fds[SIZE_TAB];
	fds[0].fd = sfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	
	for (i = 1; i < SIZE_TAB; i++) {
		fds[i].fd = -1;
		fds[i].events = POLLIN;
		fds[i].revents = 0;
	}
	while (1) {
		int nb_fds = poll(fds, SIZE_TAB , -1); // -1 pour bloquer
	if (nb_fds == -1) {
		perror("poll()");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < SIZE_TAB; i++) {
		// Activité sur la socket d'écoute : nouveau client
		if (i == 0 && (fds[0].revents & POLLIN)) {
			struct sockaddr_in client_addr;
			socklen_t addrlen = sizeof(struct sockaddr_in);
			int new_client = accept(sfd, (struct sockaddr*) &client_addr, &addrlen);
			if (new_client < 0) {
				perror("accept()");
				continue;
			}
			int fd_number = first_fds_available(fds, SIZE_TAB );
			if (fd_number == -1) {
				fprintf(stderr, "Too many clients\n");
				close(new_client);
				continue;
			}
			fds[fd_number].fd = new_client;
			fds[fd_number].events = POLLIN;
			fds[fd_number].revents = 0;
			printf("New client (ADDR %s:%hu) with FD : %d\n",
				inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), new_client);
		}
		// Activité sur une socket client : message ou déconnexion
		else if (i != 0 && fds[i].fd != -1 && (fds[i].revents & (POLLIN | POLLHUP))) {
			if (echo_server(fds[i].fd) == -1) {
				printf("Connection ended (fd %d)\n", fds[i].fd);
				close(fds[i].fd);
				fds[i].fd = -1;
			}
		}
	}
}

	printf("Start Accepting\n");

	len = sizeof(cli);
	if ((connfd = accept(sfd, (struct sockaddr*) &cli, &len)) < 0) {
		perror("accept()\n");
		exit(EXIT_FAILURE);
	}
	echo_server(connfd);
	
	
	close(sfd);
	return EXIT_SUCCESS;
}

