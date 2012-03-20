#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "kradcommander_clientlib.h"
#include "kradstation_clientlib.h"

#define BUFSIZE 8192
#define TRUE 1

#define MAX_MESSAGES 256


int listenfd, socketfd;
char *homedir;


typedef struct kradweb_St kradweb_t;
typedef struct kradweb_client_St kradweb_client_t;

kradweb_client_t *create_client(kradweb_t *kradweb);


struct kradweb_St {

	int msgnum;
	char stations[1024];
	char userlist[1024];
	kradweb_client_t *current_client;
	
	//kradweb_message_t messages[MAX_MESSAGES];
	
	pthread_rwlock_t message_lock;
	pthread_rwlock_t client_lock;
	kradweb_client_t *clients;

};

struct kradweb_client_St {

	kradweb_t *kradweb;

	char jsapifile[256];
	

	char *ptr;

	char *rest;
	char *info;

	kradstation_t *station_st;

	int msgnum;
	int fd;
	struct timeval start_tv;
	struct timeval end_tv;
	struct timezone tz;
	char station[256];
	char cmd[256];
	char resp[1024];
	char callback[256];
	char channels[1024];
	char temp_channels[1024];
	char chanuserlist[1024];
	char tempmsg[1024];
	int notfirstclientinlist;
	char command[1024];
	kradstation_ipc_client_t *kradstation;
		
	char nowplaying_string[1024];
	char message[1024];
	int message_pos;
	char handle[256];
	char channel[256];
	char *topic;
	int userset;
	int i;
	int len2;
	int len3;
	int len4;
	int len5;
	int len6;
	int wret;
	char tempchannel[256];
	char tempnick[256];
	char tempmessage[1024];

	int file_fd;
	int buflen;
	int wrote;
	int end;
	int ret;
	//int wret;
	int bufpos;
	int wrote_total;
	int length;
	int status;

	char buffer[8192];
	char outbuffer[8192];
	int commandok;
	char filename[512];
	char fullfilename[768];



	

	long req_len;

	char get[2048];
	int got;
	int len;
	int msg;
	int bytes;
	int msgbytes;
	int read_amt;

	kradweb_client_t *next;

	pthread_t client_thread;
	int pollret;
	struct pollfd fds[1];

	int stringpos;

};


void station_cmd_resp(kradweb_client_t *client, char *station_name, char *cmd, char *resp) {
	
	client->kradstation = kradstation_connect(station_name);
	
	if (client->kradstation != NULL) {
			
			
		sprintf(client->command, "%s", cmd);
			
		kradstation_cmd(client->kradstation, client->command);
		
		if (strlen(client->kradstation->krad_ipc_client->buffer) > 0) {
			
			sprintf(resp, "%s", client->kradstation->krad_ipc_client->buffer);

		}
		
		kradstation_disconnect(client->kradstation);
	
	} else {
		
	}


}

void station_cmd(kradweb_client_t *client, char *station_name, char *cmd) {
	
	client->kradstation = kradstation_connect(station_name);
	
	if (client->kradstation != NULL) {
			
		sprintf(client->command, "%s", cmd);
			
		kradstation_cmd(client->kradstation, client->command);
		
		kradstation_disconnect(client->kradstation);
	
		//printf("Krad irc: Submitted upload %s to station %s\n", filename, station_name);
	
	} else {
	
		//printf("kradirc: Krad Station client fail could not tell about upload.. %s\n", filename);
		
	}


}

void station_getinfo(kradweb_client_t *client, char *station_name, char *string) {
	
	client->kradstation = kradstation_connect(station_name);
	
	if (client->kradstation != NULL) {
			
		sprintf(client->command, "info");
			
		kradstation_cmd(client->kradstation, client->command);
		
		if (strlen(client->kradstation->krad_ipc_client->buffer) > 0) {

			//station_buffer_to_struct(client->kradstation->krad_ipc_client->buffer, &client->station_st);
			strcpy(string, client->kradstation->krad_ipc_client->buffer);

			if(strlen(client->callback)) {
				for (client->i = 0; client->i < strlen(client->kradstation->krad_ipc_client->buffer); client->i++) {
					if (string[client->i] == '\n') {
						string[client->i] = '#';
					}
				}
			}

		}





		kradstation_disconnect(client->kradstation);
	
		//printf("Krad irc: Submitted upload %s to station %s\n", filename, station_name);
	
	} else {
	
		//printf("kradirc: Krad Station client fail could not tell about upload.. %s\n", filename);
		
	}


}

kradweb_client_t *create_client(kradweb_t *kradweb) {

	//pthread_rwlock_wrlock (&kradweb->client_lock);

	kradweb_client_t *client = calloc(1, sizeof(kradweb_client_t));

	client->station_st = calloc(1, sizeof(kradstation_t));

	client->kradweb = kradweb;

	//add_client(kradweb, client);
	
	//make_userlist(kradweb);
	//client->msgnum = kradweb->msgnum; // this gets outdated
	//strcpy(client->channels, "");
	
	//pthread_rwlock_unlock (&kradweb->client_lock);
	
	return client;

}

void destroy_client(kradweb_t *kradweb, kradweb_client_t *client) {

	printf("Destroy client!!\n");
	
	//pthread_rwlock_wrlock (&kradweb->client_lock);
	

	
	close(client->fd);
	
	
	//remove_client(&client->kradweb->clients, client);

	//make_userlist(kradweb);

	//pthread_rwlock_unlock (&kradweb->client_lock);
		
	free(client->station_st);
		
	free (client);

}

void nowplaying(kradweb_client_t *client, char *station_name, char *string) {
	
	client->kradstation = kradstation_connect(station_name);
	
	if (client->kradstation != NULL) {
			
		sprintf(client->command, "current");
			
		kradstation_cmd(client->kradstation, client->command);
		
		if (strlen(client->kradstation->krad_ipc_client->buffer) > 0) {

			sprintf(string, "%s", client->kradstation->krad_ipc_client->buffer);

		}


		kradstation_disconnect(client->kradstation);
	
		//printf("Krad irc: Submitted upload %s to station %s\n", filename, station_name);
	
	} else {
	
		//printf("kradweb: Krad Station client fail could not tell about upload.. %s\n", filename);
		
	}


}


void online_stations(kradweb_client_t *client, char *string) {

	kcom_ipc_client_t *kcom;

	kcom = kcom_connect();

	if (kcom != NULL) {

			char cmd[128] = "online_stations";
	
	
			kcom_cmd(kcom, cmd);		
	
		
	
		if (strlen(kcom->krad_ipc_client->buffer) > 0) {
	
			//printf("Got online stations: %s\n", kcom->krad_ipc_client->buffer);
	
			strcpy(string, kcom->krad_ipc_client->buffer);
	
		}

		kcom_disconnect(kcom);
	
	} else {
	
		printf("Cant connect to krad commander\n");
		exit(1);
	}
}


void write_headers(kradweb_client_t *client, char *string) {

	client->stringpos += sprintf(string + client->stringpos, "HTTP/1.1 200 OK\r\n");
	//stringpos += sprintf(string, "Date: Mon, 31 Oct 2011 12:07:27 GMT\r\n");
	client->stringpos += sprintf(string + client->stringpos, "Status: 200 OK\r\n");
	client->stringpos += sprintf(string + client->stringpos, "Connection: close\r\n");
	client->stringpos += sprintf(string + client->stringpos, "Content-Type: text/plain; charset=utf-8\r\n");
	client->stringpos += sprintf(string + client->stringpos, "\r\n");

	if(strlen(client->callback)) {
		client->stringpos += sprintf(string + client->stringpos, "%s({'text': '", client->callback);
	}

}

void write_404_headers(kradweb_client_t *client, char *string) {

	client->stringpos += sprintf(string + client->stringpos, "HTTP/1.1 404 Not Found\r\n");
	//stringpos += sprintf(string, "Date: Mon, 31 Oct 2011 12:07:27 GMT\r\n");
	client->stringpos += sprintf(string + client->stringpos, "Status: 404 Not Found\r\n");
	client->stringpos += sprintf(string + client->stringpos, "Connection: close\r\n");
	client->stringpos += sprintf(string + client->stringpos, "Content-Type: text/html; charset=utf-8\r\n");
	client->stringpos += sprintf(string + client->stringpos, "\r\n");

}


int string_is_station_channel(kradweb_client_t *client, char *string) {

	if (strstr(client->kradweb->stations, string) != NULL) {
		return 1;
	}


	return 0;

}

void *web_client(void *arg) {


	kradweb_client_t *client = (kradweb_client_t *)arg;
	
	printf("\nNew Web Client\n");
	
	client->ret = read(client->fd,client->buffer,BUFSIZE); 	 
	
	if (client->ret == 0 || client->ret == -1) {	 
		printf("failed to read browser request\n");
		destroy_client(client->kradweb, client);
		pthread_exit(0);
	}
	
	client->buflen = BUFSIZE / 2;

	while(client->bufpos <= client->buflen) {
		if (strncmp(client->buffer + client->bufpos, "Content-Length: ", 15) == 0) {
			client->length = atoi(client->buffer + 15 + client->bufpos);
			printf("Upload Size: %dK\n", client->length / 1000);
		}
		
		if (strncmp(client->buffer + client->bufpos, "GET /", 5) == 0) {
		
			client->len2 = strcspn(client->buffer + client->bufpos + 5, "\r");
			client->len3 = strcspn(client->buffer + client->bufpos + 5, " ");
			client->len4 = strcspn(client->buffer + client->bufpos + 5, "?");
			client->len5 = (client->len2) > (client->len3) ? (client->len3) : (client->len2);
			client->len6 = (client->len4) > (client->len5) ? (client->len5) : (client->len4);
			memcpy(client->get, client->buffer + client->bufpos + 5, client->len6);
			client->get[client->len6] = '\0';
			
			printf("Wanted: %s\n", client->get);
			
			if ( client->len4 == client->len6) {
				if (strncmp(client->buffer + client->bufpos + 5 + client->len4 + 1, "callback=", 9) == 0) {
						client->len2 = strcspn(client->buffer + client->bufpos + 5 + client->len4 + 10, "&");
						memcpy(client->callback, client->buffer + client->bufpos + 5 + client->len4 + 10, client->len2);
						client->callback[client->len2] = '\0';
				}
			}
			
			if (strncmp("favicon.ico", client->get, 11) == 0) {
			
				printf("favicon web client done\n");

				destroy_client(client->kradweb, client);

				pthread_exit(0);

			}
			
			if (strncmp("api.js", client->get, 6) == 0) {
			

				sprintf(client->jsapifile, "%s/krad/www/api.js", homedir);

				if ((client->file_fd = open(client->jsapifile, O_RDONLY)) == -1) {
					printf("failed to js api file");
				}

				sprintf(client->outbuffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", "text/javascript");
				client->wrote = write(client->fd, client->outbuffer, strlen(client->outbuffer));

				/* send file in 2KB block - last block may be smaller */
				while (	(client->ret = read(client->file_fd, client->outbuffer, BUFSIZE)) > 0 ) {
					client->wrote = write(client->fd, client->outbuffer, client->ret);
				}

				close(client->file_fd);
			
				printf("js api web client done\n");

				destroy_client(client->kradweb, client);

				pthread_exit(0);

			}
			
			if (strncmp("api.html", client->get, 8) == 0) {
			

				sprintf(client->jsapifile, "%s/krad/www/api.html", homedir);

				if ((client->file_fd = open(client->jsapifile, O_RDONLY)) == -1) {
					printf("failed to js api html file");
				}

				sprintf(client->outbuffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", "text/html");
				client->wrote = write(client->fd, client->outbuffer, strlen(client->outbuffer));

				/* send file in 2KB block - last block may be smaller */
				while (	(client->ret = read(client->file_fd, client->outbuffer, BUFSIZE)) > 0 ) {
					client->wrote = write(client->fd, client->outbuffer, client->ret);
				}

				close(client->file_fd);
			
				printf("js api html web client done\n");

				destroy_client(client->kradweb, client);

				pthread_exit(0);

			}
			
			if (strncmp("crossdomain.xml", client->get, 15) == 0) {
			

				sprintf(client->jsapifile, "%s/krad/www/crossdomain.xml", homedir);

				if ((client->file_fd = open(client->jsapifile, O_RDONLY)) == -1) {
					printf("failed to crossdomain.xml file");
				}

				sprintf(client->outbuffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", "text/xml");
				client->wrote = write(client->fd, client->outbuffer, strlen(client->outbuffer));

				/* send file in 2KB block - last block may be smaller */
				while (	(client->ret = read(client->file_fd, client->outbuffer, BUFSIZE)) > 0 ) {
					client->wrote = write(client->fd, client->outbuffer, client->ret);
				}

				close(client->file_fd);
			
				printf("crossdomain.xml web client done\n");

				destroy_client(client->kradweb, client);

				pthread_exit(0);

			}
			
			if (strncmp("player.swf", client->get, 10) == 0) {
			

				sprintf(client->jsapifile, "%s/krad/www/player.swf", homedir);

				if ((client->file_fd = open(client->jsapifile, O_RDONLY)) == -1) {
					printf("failed to player.swf file");
				}

				sprintf(client->outbuffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", "application/x-shockwave-flash");
				client->wrote = write(client->fd, client->outbuffer, strlen(client->outbuffer));

				/* send file in 2KB block - last block may be smaller */
				while (	(client->ret = read(client->file_fd, client->outbuffer, BUFSIZE)) > 0 ) {
					client->wrote = write(client->fd, client->outbuffer, client->ret);
				}

				close(client->file_fd);
			
				printf("player.swf web client done\n");

				destroy_client(client->kradweb, client);

				pthread_exit(0);

			}
			
		}

		if (strncmp(client->buffer + client->bufpos, "X-File-Name: ", 13) == 0) {
			client->end = strcspn(client->buffer + client->bufpos + 13, "\r");
			strncpy(client->filename, client->buffer + client->bufpos + 13, client->end);
			client->filename[client->end] = '\0';
			printf("Filename: %s\n", client->filename);
		}
		
		if (strncmp(client->buffer + client->bufpos, "X-Krad-Station: ", 16) == 0) {
			strncpy(client->station, client->buffer + client->bufpos + 16, strcspn(client->buffer + client->bufpos + 16, "\r"));
			printf("Station %s\n", client->station);
		}
		
		client->bufpos += strcspn(client->buffer + client->bufpos, "\r") + 2;
		//printf("spin %d - %d\n", bufpos, buflen);
		if (strncmp(client->buffer + client->bufpos, "\r\n", 2) == 0) {
			client->bufpos = client->bufpos + 2;
			printf("Headers Length: %d\n", client->bufpos );
			break;
		}		
	}


	//printf("Headers: %s::\n", buffer);


	if (!(strlen(client->get))) {
	
		write_headers(client, client->buffer);
		client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
	
		strcpy(client->buffer, "Stations: ");
		client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
		online_stations(client, client->buffer);
		client->wrote = write(client->fd, client->buffer, strlen(client->buffer));

	} else {
	
		if (strchr(client->get, '/') != NULL) {
	
			client->len2 = strcspn(client->get, "/");
			memcpy(client->station, client->get, client->len2);
			client->station[client->len2] = '\0';
	
			client->len2 = strcspn(client->get, "/");
			strcpy(client->cmd, client->get + client->len2 + 1);
			
			printf("Station: %s Cmd: %s\n", client->station, client->cmd);
			
			if (string_is_station_channel(client, client->station)) {
	
				write_headers(client, client->buffer);
				client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
				if (!(strlen(client->cmd))) {
					nowplaying(client, client->station, client->buffer);
					client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
				} else {
								
					if (strncmp(client->cmd, "next", 4) == 0) {
						strcpy(client->buffer, "next Command OK");
						client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
						station_cmd(client, client->station, "music_cmd,next");
						client->commandok = 1;
					}
	
					if (strncmp(client->cmd, "prev", 4) == 0) {
						strcpy(client->buffer, "prev Command OK");
						client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
						station_cmd(client, client->station, "music_cmd,prev");
						client->commandok = 1;
					}
					
					if (strncmp(client->cmd, "playlist", 8) == 0) {
						station_cmd_resp(client, client->station, "music_cmd,playlist", client->buffer);
						client->wrote = write(client->fd, client->buffer, strlen(client->buffer));	
						client->commandok = 1;
					}
						
					if (!(client->commandok)) {
						station_getinfo(client, client->station, client->buffer);
						client->wrote = write(client->fd, client->buffer, strlen(client->buffer));	
					}

				}
					
			} else {
	
				write_404_headers(client, client->buffer);
				client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
				strcpy(client->buffer, "404 Not Fucking Found");
				client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
			
			}
			
	
		} else {

			if (string_is_station_channel(client, client->get)) {
	
				write_headers(client, client->buffer);
				client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
	
				nowplaying(client, client->get, client->buffer);
				client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
			} else {
	
				write_404_headers(client, client->buffer);
				client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
				strcpy(client->buffer, "404 Not Fucking Found");
				client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
			
			}
		}
	}


	if(strlen(client->callback)) {
		strcpy(client->buffer, "'})");
		client->wrote = write(client->fd, client->buffer, strlen(client->buffer));
	}


	printf("web client done\n");


	gettimeofday(&client->end_tv, &client->tz);

	client->req_len = ((client->end_tv.tv_usec - client->start_tv.tv_usec) / 1000);
	if (client->req_len > 0) {
		printf("Request took: %ldms\n", client->req_len);
	} else {
		printf("Request took: less than 1ms\n");
	}

	destroy_client(client->kradweb, client);

	pthread_exit(0);
}


void kcomweb_shutdown() {

	printf("kradweb Shutting Down\n");
	close(listenfd);
	exit(0);

}

int main(int argc, char **argv)
{
	int port, on, pri;
	socklen_t length;
	static struct sockaddr_in cli_addr;
	static struct sockaddr_in serv_addr;


	kradweb_t *kradweb = calloc(1, sizeof(kradweb_t));

	pthread_rwlock_init(&kradweb->client_lock, NULL);


	if (argc < 2) {
		printf("Krad web: plz provide a port number\n");
		exit(1);
	}

	port = atoi(argv[1]);
	
	if (port < 0 || port > 60000) {
		printf("kradweb port number error\n");
		exit(1);
	}
	
	pri = nice(10);
	
	printf("Krad web Starting Up on port %d, nice is %d\n", port, pri);
	
	signal(SIGPIPE, SIG_IGN); /* ignore writing to a boken connection */
	signal(SIGCLD, SIG_IGN); /* ignore child death */
	signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */

	signal(SIGINT, kcomweb_shutdown);
	signal(SIGTERM, kcomweb_shutdown);


	kcom_ipc_client_t *kcom;

	kcom = kcom_connect();

	if (kcom != NULL) {

			char cmd[128] = "online_stations";
	
	
			kcom_cmd(kcom, cmd);		
	
		
	
		if (strlen(kcom->krad_ipc_client->buffer) > 0) {
	
			printf("Got online stations: %s\n", kcom->krad_ipc_client->buffer);
	
			strcpy(kradweb->stations, kcom->krad_ipc_client->buffer);
	
		}

		kcom_disconnect(kcom);
	
	} else {
	
		printf("Cant connect to krad commander\n");
		exit(1);
	}

	homedir = getenv ("HOME");
 	
	/* setup the network socket */
	if ((listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0) {
		printf("kradweb system call socket error\n");
		exit(1);
	}
	
	on = 1;
	
	if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) < 0)
	{
		printf("kradweb system call setsockopt error");
		close(listenfd);
		exit (1);
	}	
	
	if (port < 0 || port > 60000) {
		printf("kradweb port number error\n");
		exit(1);
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) {
		printf("kradweb system call bind error\n");
		exit(1);
	}

	if( listen(listenfd,64) <0) {
		printf("kradweb system call listen error\n");
		exit(1);
	}

	while (TRUE) {
	
	
		kradweb_client_t *newclient = create_client(kradweb);
	
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0) {
			close(listenfd);
			printf("kradweb socket error on accept mayb a signal or such\n");
			exit(1);
		}

		newclient->fd = socketfd;

		// update the msgnum since it was created before the fact
		newclient->msgnum = kradweb->msgnum;


		gettimeofday(&newclient->start_tv, &newclient->tz);


		pthread_create(&newclient->client_thread, NULL, web_client, (void *)newclient);
		pthread_detach(newclient->client_thread);


/*
		if ((pid = fork()) < 0) {
			printf("kradweb syscall error on fork\n");
			exit(1);
		}
		
		else {
			if(pid == 0) {
				close(listenfd);
				//upload(socketfd);
				chat(socketfd);
			} else {
				close(socketfd);
			}
		}
*/


	}
}
