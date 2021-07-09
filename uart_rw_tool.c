#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "uart.h"

void setTermios(struct termios * pNewtio, int uBaudRate)
{
	bzero(pNewtio, sizeof(struct termios)); /* clear struct for new port settings */
	//8N1
	pNewtio->c_cflag = uBaudRate | CS8 | CREAD | CLOCAL;
	pNewtio->c_iflag = IGNPAR;
	pNewtio->c_oflag = 0;
	pNewtio->c_lflag = 0; //non ICANON

	/*

	initialize all control characters

	default values can be found in /usr/include/termios.h, and

	are given in the comments, but we don't need them here

	*/

	pNewtio->c_cc[VINTR] = 0; /* Ctrl-c */
	pNewtio->c_cc[VQUIT] = 0; /* Ctrl-\ */
	pNewtio->c_cc[VERASE] = 0; /* del */
	pNewtio->c_cc[VKILL] = 0; /* @ */
	pNewtio->c_cc[VEOF] = 4; /* Ctrl-d */
	pNewtio->c_cc[VTIME] = 5; /* inter-character timer, timeout VTIME*0.1 */
	pNewtio->c_cc[VMIN] = 0; /* blocking read until VMIN character arrives */
	pNewtio->c_cc[VSWTC] = 0; /* '\0' */
	pNewtio->c_cc[VSTART] = 0; /* Ctrl-q */
	pNewtio->c_cc[VSTOP] = 0; /* Ctrl-s */
	pNewtio->c_cc[VSUSP] = 0; /* Ctrl-z */
	pNewtio->c_cc[VEOL] = 0; /* '\0' */
	pNewtio->c_cc[VREPRINT] = 0; /* Ctrl-r */
	pNewtio->c_cc[VDISCARD] = 0; /* Ctrl-u */
	pNewtio->c_cc[VWERASE] = 0; /* Ctrl-w */
	pNewtio->c_cc[VLNEXT] = 0; /* Ctrl-v */
	pNewtio->c_cc[VEOL2] = 0; /* '\0' */
}

// Usage Example: ./rw /dev/ttyUSB0 9600 3 C1
int main(int argc, char* argv[]) {
	int fd, len, speed, message_index;
	struct termios oldtio, newtio;


	if (argc != 4 && argc != 5) {
		fprintf(stderr, "Usage: %s <uart_device> <Baud_rate> <echo OR number of message> <message>\n", argv[0]);
		exit(1);
	}
	
	if ((fd = open(argv[1], O_RDWR | O_NOCTTY)) < 0) {
		perror("uart_fdket");
		exit(1);
	}

	tcgetattr(fd, &oldtio); /* save current serial port settings */
	setTermios(&newtio, B115200);
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);
	
	speed = atoi(argv[2]);
	set_speed(fd, speed);
	
	if(set_parity(fd, 8, 1, 'N') == FALSE) {
		fprintf(stderr, "Set Parity Error\n");
		exit(0);
	}
	
	if(strcmp(argv[3],"echo") != 0) {	
		unsigned char message = strtol(argv[4], NULL, 16);
		int n;
		sscanf(argv[3], "%d", &n);
		printf("writing information:\n");
		printf("numbers of message will be sent: %d \nmessage: %hhx\n", n, message); 
		for(int i=0; i<n; i++) {
			len = write(fd, &message, 1);
			if(len > 0) printf("send message success\n");
			else printf("send message failed\n");
		}
		printf("====================================\n");
	}
	else {
		len = write(fd, argv[4], (strlen(argv[4])+1));
		if (len <= 0) {
			printf("send data failed\n");
		}
		else {
			printf("send data success\n");
		}
	}

	if (strcmp(argv[3], "echo") == 0) {
		char buf[100];
		struct timeval tv;

		tv.tv_sec = 1; // waiting at most 1 seconds for echo
		tv.tv_usec = 0;

		fd_set all_fds;
		fd_set listen_fds;
		FD_ZERO(&all_fds);
		FD_SET(fd, &all_fds);

		memset(buf, 0, 100);
		buf[0] = '\0';

		printf("waiting for echo...\n");

		if(select(1+fd, &all_fds, NULL, NULL, &tv) > 0) {
			sleep(1);
			if(FD_ISSET(fd, &all_fds)) {
				len = read(fd, buf, 100);
				buf[len] = '\0';
			}
		}
		
		if (len > 0) {
			printf("read message: %s\n", buf);
		}
		else printf("No echo, please check the wiring\n");

	}
	else {
		//char buf[100];
		unsigned char nums[100];
		struct timeval tv;

		tv.tv_sec = 3; // waiting at most 3 seconds for response
		tv.tv_usec = 0;

		fd_set all_fds;
		fd_set listen_fds;
		FD_ZERO(&all_fds);
		FD_SET(fd, &all_fds);

		memset(nums,0,100);

		printf("waiting for response message...\n");

		if(select(1+fd, &all_fds, NULL, NULL, &tv) > 0) {
			sleep(1);
			if(FD_ISSET(fd, &all_fds)) {
				len = read(fd, nums, 100);
			}
			printf("length: %d\n", len);
			for(int i=0; i<len; i++) printf("%02hhx ", nums[i]);
			printf("\n");
		}
		else printf("read nothing\n");

	}
	close(fd);

	return 0;
}
