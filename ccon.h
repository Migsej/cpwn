#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>   
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>

typedef enum {
	PROCESS_REMOTE,
	PROCESS_LOCAL,
} PROCESS_KIND;

typedef struct {
	PROCESS_KIND kind;	
	union {
		int fd;
		struct {
			int pipe_to_program[2];
			int pipe_from_program[2];
		};
	};
} proc;

proc remote(char *address, unsigned short port);
proc process(char *program);
int closep(proc conn);

int sendline(proc conn, char *message);

//uses malloc
char *recvuntil(proc conn, char *message);
char *recvline(proc conn);


#ifdef CPWN_IMPLEMENTATION

proc remote(char *address, unsigned short port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(1); 
	} else {
		printf("socket creation succeded!!\n"); 
	}
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(address);
	servaddr.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
		printf("connection with the server failed...\n");
		exit(1);
	}
	else {
		printf("connected to the server\n");
	}

	proc result = {0};
	result.kind = PROCESS_REMOTE;
	result.fd = sockfd;
	return result;
}

proc process(char *program) {
	int pipe_to_program[2] ;
	int pipe_from_program[2] ;
	if (pipe(pipe_to_program) == -1 || pipe(pipe_from_program) == -1) {
		printf("Pipe failed");
		exit(1);
	}

	pid_t pid = fork();
	if (pid < 0) {
		printf("Fork failed");
		exit(1);
	}
	if (pid > 0) {  // Parent process
		close(pipe_to_program[0]);  
		close(pipe_from_program[1]);  

		proc result;
		result.kind = PROCESS_LOCAL;
		memcpy(result.pipe_to_program, pipe_to_program, sizeof(pipe_to_program));
		memcpy(result.pipe_from_program, pipe_from_program, sizeof(pipe_from_program));

		return result;

	} else {  // Child process
		close(pipe_to_program[1]);  
		close(pipe_from_program[0]);  

		dup2(pipe_to_program[0], STDIN_FILENO);
		dup2(pipe_from_program[1], STDOUT_FILENO);

		execlp(program, program, NULL);

		perror("spawning process failed");
		exit(EXIT_FAILURE);
	}
}

int sendline(proc conn, char *message) {
	long length = strlen(message);
	char newlined[length + 1];
	strcpy(newlined, message);
	newlined[length] = '\n';
	if (conn.kind == PROCESS_REMOTE) {
		if (send(conn.fd, newlined, length+1, 0 ) < 0) {
			printf("no good sending message to remote: %s\n", message);
			return -1;
		}
	} else if (conn.kind == PROCESS_LOCAL) {
		if (write(conn.pipe_to_program[1], newlined, length+1) < 0) {
			printf("no good sending message to process: %s\n", message);
			return -1;
		}
	} else {
		return -1;
	}
	return 1;
}

char *recvuntil(proc conn, char *message) {
	int strlength = strlen(message);
	char *result = malloc(strlength * sizeof(char));
	int length = 0;
	int capacity = strlength;
	char buffer[1];
	while ( length < strlength || strncmp(message, (result + length)-strlength, strlength) != 0) {
		switch (conn.kind) {
			case PROCESS_REMOTE:
				if (read(conn.fd, buffer, 1) == -1) {
					printf("could not read");
					return NULL;
				}
				break;
			case PROCESS_LOCAL:
				if (read(conn.pipe_from_program[0], buffer, 1) == -1) {
					printf("could not read");
					return NULL;
				}
				break;
		}
		length++;
		if (capacity < length) {
			capacity *= 2;
			if (NULL == realloc(result, capacity * sizeof(*result))) {
				printf("problem allocation result");
				return NULL;
			};
		}
		result[length-1] = buffer[0];	
	} 
	return result;
}

char *recvline(proc conn) {
	return recvuntil(conn, "\n");
}

int closep(proc conn) {
	if (conn.kind == PROCESS_REMOTE) {
		if ( 0 > close(conn.fd)) {
			perror("could not close local");
			return 0;
		}
	} else {
		if ((0 > close(conn.pipe_to_program[1]) || (0 > close(conn.pipe_from_program[0])))) {
			perror("could not close remote");
			return 0;
		}
	}
}

#endif //CPWN_IMPLEMENTATION
