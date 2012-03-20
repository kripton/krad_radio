#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "kradstation_clientlib.h"

#define BUFSIZE 8192
#define TRUE 1

int listenfd, socketfd;
char *homedir;

void tell_station_about_upload(char *station_name, char *filename) {

	char command[1024];
	kradstation_ipc_client_t *kradstation;
	
	kradstation = kradstation_connect(station_name);
	
	if (kradstation != NULL) {
			
		sprintf(command, "importfile,%s", filename);
			
		kradstation_cmd(kradstation, command);

		kradstation_disconnect(kradstation);
	
		printf("Krad Uploader: Submitted upload %s to station %s\n", filename, station_name);
	
	} else {
	
		printf("kraduploader: Krad Station client fail could not tell about upload.. %s\n", filename);
		
	}


}


void upload(int fd)
{
	int file_fd = 0, buflen = 0, wrote = 0, end = 0, ret = 0, bufpos = 0, wrote_total = 0, length = 0, status;
	
	static char buffer[8192];
 
 	char station[256];
 	char filename[512];
	char fullfilename[768];

	struct stat st;

	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	
	printf("\nNew Upload\n");
	
	ret = read(fd,buffer,BUFSIZE); 	 
	
	if (ret == 0 || ret == -1) {	 
		printf("failed to read browser request\n");
		exit(1);
	}
	
	buflen = BUFSIZE / 2;

	while(bufpos <= buflen) {
		if (strncmp(buffer + bufpos, "Content-Length: ", 15) == 0) {
			length = atoi(buffer + 15 + bufpos);
			printf("Upload Size: %dK\n", length / 1000);
		}

		if (strncmp(buffer + bufpos, "X-File-Name: ", 13) == 0) {
			end = strcspn(buffer + bufpos + 13, "\r");
			strncpy(filename, buffer + bufpos + 13, end);
			filename[end] = '\0';
			printf("Filename: %s\n", filename);
		}
		
		if (strncmp(buffer + bufpos, "X-Krad-Station: ", 16) == 0) {
			strncpy(station, buffer + bufpos + 16, strcspn(buffer + bufpos + 16, "\r"));
			printf("Station %s\n", station);
		}
		
		bufpos += strcspn(buffer + bufpos, "\r") + 2;
		//printf("spin %d - %d\n", bufpos, buflen);
		if (strncmp(buffer + bufpos, "\r\n", 2) == 0) {
			bufpos = bufpos + 2;
			printf("Headers Length: %d\n", bufpos );
			break;
		}		
	}


	// test dir existance
	sprintf(fullfilename, "%s/krad/uploads/%s", homedir, station);
	if(stat(fullfilename, &st) == 0) {
		printf("Directory Exists: %s\n", fullfilename );
	} else {
		status = mkdir(fullfilename, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (status == 0) {	
			printf("Directory Created: %s\n", fullfilename );
		} else {
			printf("Cant create directory: %s\n", fullfilename );
		}	
	}

	sprintf(fullfilename, "%s/krad/uploads/%s/%s", homedir, station, filename);

	if ((file_fd = open(fullfilename, O_WRONLY | O_CREAT, mode)) == -1) {
		printf("failed to open new file");
		exit(1);
	}


	wrote = write(file_fd, buffer + bufpos, ret - bufpos);
	wrote_total += wrote;
	//printf("wrote %d bytes total %d\n", wrote, wrote_total);

	while (wrote_total != length) {

		if (wrote_total > length) {	 
			printf("failed got to much data\n");
			close(file_fd);
			close(fd);
			exit(1);
		}

		ret = read(fd, buffer, BUFSIZE);
		
		if (ret == 0 || ret < 0) {	 
			printf("failed to read incomping socket\n");
			close(file_fd);
			close(fd);
			exit(1);
		}
		
		wrote = write(file_fd, buffer, ret);
		wrote_total += wrote;
	}

	close(file_fd);
	close(fd);

	printf("Upload Complete %s - %dKB / %dMB for station %s\n", fullfilename, wrote_total / 1000, (wrote_total / 1000 / 1000), station);

	tell_station_about_upload(station, fullfilename);


	exit(0);
}


void kcomweb_shutdown() {

	printf("kraduploader Shutting Down\n");
	close(listenfd);
	exit(0);

}

int main(int argc, char **argv)
{
	int port, pid, on, pri;
	socklen_t length;
	static struct sockaddr_in cli_addr;
	static struct sockaddr_in serv_addr;

	if (argc < 2) {
		printf("Krad Uploaded: plz provide a port number\n");
		exit(1);
	}

	port = atoi(argv[1]);
	
	if (port < 0 || port > 60000) {
		printf("kraduploader port number error\n");
		exit(1);
	}
	
	pri = nice(10);
	
	printf("Krad Uploader Starting Up on port %d, nice is %d\n", port, pri);
	
	signal(SIGCLD, SIG_IGN); /* ignore child death */
	signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */

	signal(SIGINT, kcomweb_shutdown);
	signal(SIGTERM, kcomweb_shutdown);

	homedir = getenv ("HOME");
 	
	/* setup the network socket */
	if ((listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0) {
		printf("kraduploader system call socket error\n");
		exit(1);
	}
	
	on = 1;
	
	if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) < 0)
	{
		printf("kraduploader system call setsockopt error");
		close(listenfd);
		exit (1);
	}	
	
	if (port < 0 || port > 60000) {
		printf("kraduploader port number error\n");
		exit(1);
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) {
		printf("kraduploader system call bind error\n");
		exit(1);
	}

	if( listen(listenfd,64) <0) {
		printf("kraduploader system call listen error\n");
		exit(1);
	}

	while (TRUE) {
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0) {
			close(listenfd);
			printf("kraduploader socket error on accept mayb a signal or such\n");
			exit(1);
		}

		if ((pid = fork()) < 0) {
			printf("kraduploader syscall error on fork\n");
			exit(1);
		}
		
		else {
			if(pid == 0) {
				close(listenfd);
				upload(socketfd);
			} else {
				close(socketfd);
			}
		}
	}
}
