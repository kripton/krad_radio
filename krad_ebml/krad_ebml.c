/*
 * Copyright © 2010 Mozilla Foundation
 *
 * This program is made available under an ISC-style license.  See the
 * accompanying file LICENSE for details.
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "halloc.h"

#include "krad_ebml.h"


char *kradebml_version() {

	return KRADEBML_VERSION;

}


unsigned char get_next_match_byte(unsigned char match_byte, uint64_t position, uint64_t *matched_byte_num, uint64_t *winner) {

	if (winner != NULL) {
		*winner = 0;
	}

	if (matched_byte_num != NULL) {
		if (match_byte == KRAD_EBML_CLUSTER_BYTE1) {
			if (matched_byte_num != NULL) {
				*matched_byte_num = position;
			}
			return KRAD_EBML_CLUSTER_BYTE2;
		}
	
		if ((*matched_byte_num == position - 1) && (match_byte == KRAD_EBML_CLUSTER_BYTE2)) {
			return KRAD_EBML_CLUSTER_BYTE3;
		}
	
		if ((*matched_byte_num == position - 2) && (match_byte == KRAD_EBML_CLUSTER_BYTE3)) {
			return KRAD_EBML_CLUSTER_BYTE4;
		}	

		if ((*matched_byte_num == position - 3) && (match_byte == KRAD_EBML_CLUSTER_BYTE4)) {
			if (winner != NULL) {
				*winner = *matched_byte_num;
			}
			*matched_byte_num = 0;
			return KRAD_EBML_CLUSTER_BYTE1;
		}

		*matched_byte_num = 0;
	}
	
	return KRAD_EBML_CLUSTER_BYTE1;

}

void krad_ebml_destroy(krad_ebml_t *krad_ebml) {

	free(krad_ebml->buffer);
	free(krad_ebml->header);
	free(krad_ebml);

}

krad_ebml_t *krad_ebml_create_feedbuffer() {

	krad_ebml_t *krad_ebml = calloc(1, sizeof(krad_ebml_t));

	krad_ebml->buffer = calloc(1, KRAD_EBML_BUFFER_SIZE);
	krad_ebml->header = calloc(1, KRAD_EBML_HEADER_MAX_SIZE);

	
	krad_ebml->cluster_mark[0] = KRAD_EBML_CLUSTER_BYTE1;
	krad_ebml->cluster_mark[1] = KRAD_EBML_CLUSTER_BYTE2;
	krad_ebml->cluster_mark[2] = KRAD_EBML_CLUSTER_BYTE3;
	krad_ebml->cluster_mark[3] = KRAD_EBML_CLUSTER_BYTE4;

	return krad_ebml;

}

size_t krad_ebml_read_space(krad_ebml_t *krad_ebml) {

	size_t read_space;
	
	if (krad_ebml->header_read == 1) {
		read_space = (krad_ebml->position - krad_ebml->header_size) - krad_ebml->read_position;
	
		return read_space;
	} else {
		if (krad_ebml->header_size != 0) {
			return krad_ebml->header_size - krad_ebml->header_read_position;
		} else {
			return 0;
		}
	}


}

int krad_ebml_read(krad_ebml_t *krad_ebml, char *buffer, int len) {

	size_t read_space;
	size_t read_space_to_cluster;
	int to_read;
	
	read_space_to_cluster = 0;
	
	if (len < 1) {
		return 0;
	}
	
	if (krad_ebml->header_read == 1) {
		read_space = (krad_ebml->position - krad_ebml->header_size) - krad_ebml->read_position;
	
		if (read_space < 1) {
			return 0;
		}
		
		if (read_space >= len ) {
			to_read = len;
		} else {
			to_read = read_space;
		}
		
		if (krad_ebml->cluster_position != 0) {
			read_space_to_cluster = (krad_ebml->cluster_position - krad_ebml->header_size) - krad_ebml->read_position;
	//		printf("cluster pos %zu rstc: %zu to_read %d\n", krad_ebml->cluster_position,  read_space_to_cluster, to_read);
			if ((read_space_to_cluster != 0) && (read_space_to_cluster <= to_read)) {
				to_read = read_space_to_cluster;
				krad_ebml->cluster_position = 0;
				krad_ebml->last_was_cluster_end = 1;
			} else {
				if (read_space_to_cluster == 0) {
					krad_ebml->this_was_cluster_start = 1;
				}
			}
		}
	
		memcpy(buffer, krad_ebml->buffer, to_read);
		krad_ebml->read_position += to_read;
		

		memmove(krad_ebml->buffer, krad_ebml->buffer + to_read, krad_ebml->buffer_position_internal - to_read);
		krad_ebml->buffer_position_internal -= to_read;
		
	} else {
		if (krad_ebml->header_size != 0) {
			
			read_space = krad_ebml->header_size - krad_ebml->header_read_position;
	
			if (read_space >= len ) {
				to_read = len;
			} else {
				to_read = read_space;
			}
	
			memcpy(buffer, krad_ebml->header, to_read);
			krad_ebml->header_read_position += to_read;		
			
			if (krad_ebml->header_read_position == krad_ebml->header_size) {
				krad_ebml->header_read = 1;
			}
			
		} else {
			return 0;
		}
	}
	
	
	return to_read;	

}

int krad_ebml_last_was_sync(krad_ebml_t *krad_ebml) {

	if (krad_ebml->last_was_cluster_end == 1) {
		krad_ebml->last_was_cluster_end = 0;
		krad_ebml->this_was_cluster_start = 1;
	}
	
	if (krad_ebml->this_was_cluster_start == 1) {
		krad_ebml->this_was_cluster_start = 0;
		return 1;
	}

	return 0;

}

char *krad_ebml_write_buffer(krad_ebml_t *krad_ebml, int len) {

	krad_ebml->input_buffer = malloc(len);

	return (char *)krad_ebml->input_buffer;


}


int krad_ebml_wrote(krad_ebml_t *krad_ebml, int len) {

	//start push
	
	//printf("\nwrote with %d bytes\n", len);
	
	int b;

	for (b = 0; b < len; b++) {
		if ((krad_ebml->input_buffer[b] == krad_ebml->match_byte) || (krad_ebml->matched_byte_num > 0)) {
			krad_ebml->match_byte = get_next_match_byte(krad_ebml->input_buffer[b], krad_ebml->position + b, &krad_ebml->matched_byte_num, &krad_ebml->found);
			if (krad_ebml->found > 0) {
				if (krad_ebml->header_size == 0) {
					if (b > 0) {
						memcpy(krad_ebml->header + krad_ebml->header_position, krad_ebml->input_buffer, b);
						krad_ebml->header_position += b;
					}
					krad_ebml->header_size = (krad_ebml->header_position - 4) + 1;
					printf("\ngot header, size is %d\n", krad_ebml->header_size);

					// first cluster
					memcpy(krad_ebml->buffer, krad_ebml->cluster_mark, 4);
					krad_ebml->buffer_position_internal += 4;
					if ((b + 1) < len) {
						memcpy(krad_ebml->buffer + krad_ebml->buffer_position_internal, krad_ebml->input_buffer + (b + 1), len - (b + 1));
						krad_ebml->buffer_position_internal += len - (b + 1);
					}
					
					printf("\nfound first cluster starting at %zu\n", krad_ebml->found);
					krad_ebml->cluster_position = krad_ebml->found;
					krad_ebml->position += len;
					free(krad_ebml->input_buffer);
					return len;

				}
				printf("\nfound cluster starting at %zu\n", krad_ebml->found);
				krad_ebml->cluster_position = krad_ebml->found;
			}
		}
	}
	
	if (krad_ebml->header_size == 0) {
		printf("\nadding to header header, pos is %d size is %d adding %d\n", krad_ebml->header_size, krad_ebml->header_position, len);
		memcpy(krad_ebml->header + krad_ebml->header_position, krad_ebml->input_buffer, len);
		krad_ebml->header_position += len;
	} else {
	
		memcpy(krad_ebml->buffer + krad_ebml->buffer_position_internal, krad_ebml->input_buffer, len);
		krad_ebml->buffer_position_internal += len;
	}
	
	krad_ebml->position += len;

	//end push

	free(krad_ebml->input_buffer);

	return len;
}

void base64_encode(char *dest, char *src) {

	base64_t *base64 = calloc(1, sizeof(base64_t));

	static char base64table[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
									'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 
									'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', 
									'5', '6', '7', '8', '9', '+', '/' };

	base64->len = strlen(src);
	base64->out = (char *) malloc(base64->len * 4 / 3 + 4);
	base64->result = base64->out;

	while(base64->len > 0) {
		base64->chunk = (base64->len > 3) ? 3 : base64->len;
		*base64->out++ = base64table[(*src & 0xFC) >> 2];
		*base64->out++ = base64table[((*src & 0x03) << 4) | ((*(src + 1) & 0xF0) >> 4)];
		switch(base64->chunk) {
			case 3:
				*base64->out++ = base64table[((*(src + 1) & 0x0F) << 2) | ((*(src + 2) & 0xC0) >> 6)];
				*base64->out++ = base64table[(*(src + 2)) & 0x3F];
				break;

			case 2:
				*base64->out++ = base64table[((*(src + 1) & 0x0F) << 2)];
				*base64->out++ = '=';
				break;

			case 1:
				*base64->out++ = '=';
				*base64->out++ = '=';
				break;
		}

		src += base64->chunk;
		base64->len -= base64->chunk;
	}

	*base64->out = 0;

	strcpy(dest, base64->result);
	free(base64->result);
	free(base64);

}

void server_build_http_header(server_t *server) {


	strcpy(server->content_type, "");
	
	//if ((strstr(krad_mkvsource->filename, ".webm") != NULL) || (strstr(krad_mkvsource->filename, ".WEBM") != NULL)) {
		strcpy(server->content_type, "video/webm");
	//} 
	/*
	if ((strstr(krad_mkvsource->filename, ".mkv") != NULL) || (strstr(krad_mkvsource->filename, ".MKV") != NULL)) {
		strcpy(krad_mkvsource->content_type, "video/x-matroska");
	}

	if ((strstr(krad_mkvsource->filename, ".mka") != NULL) || (strstr(krad_mkvsource->filename, ".MKA") != NULL)) {
		strcpy(krad_mkvsource->content_type, "audio/x-matroska");
	} 
	
	if ((strstr(krad_mkvsource->filename, ".mk3d") != NULL) || (strstr(krad_mkvsource->filename, ".MK3D") != NULL)) {
		strcpy(krad_mkvsource->content_type, "video/x-matroska-3d");
	} 
	*/
	//if ((strstr(krad_mkvsource->filename, ".krado") != NULL) || (strstr(krad_mkvsource->filename, ".KRADO") != NULL)) {
	//	strcpy(server->content_type, "audio/x-krad-opus");
	//} 
	/*
	if ((strstr(krad_mkvsource->filename, ".mp3") != NULL) || (strstr(krad_mkvsource->filename, ".MP3") != NULL)) {
		strcpy(krad_mkvsource->content_type, "audio/mpeg");
	} 
	
	if ((strstr(krad_mkvsource->filename, ".ogg") != NULL) || (strstr(krad_mkvsource->filename, ".OGG") != NULL)) {
		strcpy(krad_mkvsource->content_type, "application/ogg");
	}
	*/
	//if (strlen(krad_mkvsource->content_type) == 0) {
	//	fprintf(stderr, "Krad MKV Source: Could not determine content type\n");
	//	krad_mkvsource_fail();
	//}


	if (server->type == ICECAST2) {

		sprintf( server->auth, "source:%s", server->password );
		base64_encode( server->auth_base64, server->auth );

		//sprintf( server->audio_info, "ice-samplerate=%d;ice-bitrate=%d;ice-channels=%d", 
		//											 DEFAULT_SAMPLERATE, DEFAULT_BITRATE, DEFAULT_CHANNELS );

		server->header_pos = sprintf( server->headers, "SOURCE %s ICE/1.0\r\n", server->mount);
		server->header_pos += sprintf( server->headers + server->header_pos, "content-type: %s\r\n", server->content_type);
		server->header_pos += sprintf( server->headers + server->header_pos, "Authorization: Basic %s\r\n", server->auth_base64);
		
		/*
		server->header_pos += sprintf( server->headers + server->header_pos, "ice-name: %s\n", DEFAULT_STREAMNAME);
		server->header_pos += sprintf( server->headers + server->header_pos, "ice-url: %s\n", DEFAULT_URL);
		server->header_pos += sprintf( server->headers + server->header_pos, "ice-genre: %s\n", "Rock");
		server->header_pos += sprintf( server->headers + server->header_pos, "ice-private: %d\n", !DEFAULT_PUBLIC);
		server->header_pos += sprintf( server->headers + server->header_pos, "ice-public: %d\n", DEFAULT_PUBLIC);
		server->header_pos += sprintf( server->headers + server->header_pos, "ice-description: %s\n", DEFAULT_DESCRIPTION);
		server->header_pos += sprintf( server->headers + server->header_pos, "ice-bitrate: %d\n", DEFAULT_BITRATE);
		server->header_pos += sprintf( server->headers + server->header_pos, "ice-audio-info: %s\n\n", server->audio_info);
		*/
		
		server->header_pos += sprintf( server->headers + server->header_pos, "\r\n");
		
	}
}

server_t *server_create(char *host, int port, char *mount, char *password) {

	server_t *server = calloc(1, sizeof(server_t));

	server->port = port;
	strcpy(server->host, host);
	strcpy(server->password, password);
	strcpy(server->mount, mount);
	server->type = ICECAST2;

	return server;

}

void server_destroy(server_t *server) {

	server_disconnect(server);
	free(server);

}

void server_disconnect(server_t *server) {

	server->connected = 0;
	close(server->sd);

}

int server_connect(server_t *server) {

	printf("Krad MKV Source: Connecting to %s:%d\n", server->host, server->port);

	if ((server->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Krad MKV Source: Socket Error\n");
		exit(1);
	}

	memset(&server->serveraddr, 0x00, sizeof(struct sockaddr_in));
	server->serveraddr.sin_family = AF_INET;
	server->serveraddr.sin_port = htons(server->port);
	
	if ((server->serveraddr.sin_addr.s_addr = inet_addr(server->host)) == (unsigned long)INADDR_NONE)
	{
		// get host address 
		server->hostp = gethostbyname(server->host);
		if(server->hostp == (struct hostent *)NULL)
		{
			printf("Krad MKV Source: Mount problem\n");
			close(server->sd);
			exit(1);
		}
		memcpy(&server->serveraddr.sin_addr, server->hostp->h_addr, sizeof(server->serveraddr.sin_addr));
	}

	// connect() to server. 
	if((server->sent = connect(server->sd, (struct sockaddr *)&server->serveraddr, sizeof(server->serveraddr))) < 0)
	{
		printf("Krad MKV Source: Connect Error\n");
		exit(1);
	} else {
	
		server->connected = 1;
		printf("Krad MKV Source: Connected.\n");
	}
	
	return server->sd;
	
}



void server_send(server_t *server, char *buffer, int length) {

	server->sent = 0;	

	int sended;

	printf("sending %d bytes\n", length);

	while (server->sent != length) {

		sended = send (server->sd, 
										buffer + server->sent, 
										length - server->sent, 0);

		if (sended <= 0) {
			fprintf(stderr, "Krad MKV Source: send Got Disconnected from server\n");
				
			exit(1);
		}
		
		server->sent += sended;	

		server->total_bytes_sent += server->sent;

	}
}


static int ke_io_write(nestegg_io * io, void * buffer, size_t length);

//////////////////////////////////////

int kradebml_feedbuffer_open(void *userdata)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

		
		
	return 0;
}

int kradebml_feedbuffer_close(void *userdata)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

		
		
	return 0;
}

int kradebml_feedbuffer_write(void *buffer, size_t length, void *userdata)
{
	size_t r;

	kradebml_t *kradebml = (kradebml_t *)userdata;

		
		
	return 1;
}

int kradebml_feedbuffer_read(void *buffer, size_t length, void *userdata)
{
	size_t r;

	kradebml_t *kradebml = (kradebml_t *)userdata;

	int bytes = 0;


	while ((bytes != length)  && (kradebml->shutdown == 0)) {

		if (kradebml->shutdown == 1) {
			printf("feedbuffer read shutd down\n");
			exit(0);
		}


		//bytes += jack_ringbuffer_read(kradebml->input_ring, buffer + bytes, length - bytes);

		if (bytes != length) {
			//printf("waiting for moar bytes\n");
			usleep(10000);
			
		}

		kradebml->total_bytes_read += bytes;

	}
	
					//printf("read %d bytes\n", bytes);
		
	return 1;
}

int kradebml_feedbuffer_seek(int64_t offset, int whence, void *userdata)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

	//printf("seek happened to %ld %d\n", offset, whence);

	return fseek(kradebml->fp, offset, whence);
}

int64_t kradebml_feedbuffer_tell(void *userdata)
{
	int64_t loc;
	kradebml_t *kradebml = (kradebml_t *)userdata;

	return kradebml->total_bytes_read;
}

////////////////////

int kradebml_stream_open(void *userdata)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

	server_connect(kradebml->server);
		
	return 0;
}

int kradebml_stream_close(void *userdata)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

	server_disconnect(kradebml->server);
		
	return 0;
}

int kradebml_stream_write(void *buffer, size_t length, void *userdata)
{
	size_t r;

	kradebml_t *kradebml = (kradebml_t *)userdata;

	server_send(kradebml->server, buffer, length);
		
	return 1;
}

int kradebml_stream_read(void *buffer, size_t length, void *userdata)
{
	size_t r;

	kradebml_t *kradebml = (kradebml_t *)userdata;

	int bytes = 0;


	while (bytes != length) {

		bytes += recv (kradebml->server->sd, 
										buffer + bytes, 
										length - bytes, 0);

		if (bytes <= 0) {
			fprintf(stderr, "Krad MKV Source: recv Got Disconnected from server\n");
				exit(1);
			
		} else {
		
			printf("got %d bytes\n", bytes);
		
		}

		//server->total_bytes_sent += server->sent;

	}
		
	return 1;
}

int kradebml_stream_seek(int64_t offset, int whence, void *userdata)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

	//printf("seek happened to %ld %d\n", offset, whence);

	return fseek(kradebml->fp, offset, whence);
}

int64_t kradebml_stream_tell(void *userdata)
{
	int64_t loc;
	kradebml_t *kradebml = (kradebml_t *)userdata;
	
	//loc = ftell(kradebml->fp);

	return loc;
}


int kradebml_stdio_open(void *userdata, char *mode)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

	kradebml->fp = fopen(kradebml->filename, mode);
		
	return 0;
}

int kradebml_stdio_close(void *userdata)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

	fclose(kradebml->fp);
		
	return 0;
}

int kradebml_stdio_write(void *buffer, size_t length, void *userdata)
{
	size_t r;

	kradebml_t *kradebml = (kradebml_t *)userdata;

	r = fwrite(buffer, length, 1, kradebml->fp);
	if (r == 0 && feof(kradebml->fp))
		return 0;
		
	return r == 0 ? -1 : 1;
}

int kradebml_stdio_read(void *buffer, size_t length, void *userdata)
{
	size_t r;

	kradebml_t *kradebml = (kradebml_t *)userdata;

	r = fread(buffer, length, 1, kradebml->fp);

	kradebml->total_bytes_read += length;

	if (r == 0 && feof(kradebml->fp))
		return 0;
		
	return r == 0 ? -1 : 1;
}

int kradebml_stdio_seek(int64_t offset, int whence, void *userdata)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

	printf("seek happened to %ld %d\n", offset, whence);

	return fseek(kradebml->fp, offset, whence);
}

int64_t kradebml_stdio_tell(void *userdata)
{
	int64_t loc;
	kradebml_t *kradebml = (kradebml_t *)userdata;
	
	loc = ftell(kradebml->fp);

	return loc;
}

static void
cluster_at(int64_t offset, void *userdata)
{

	kradebml_t *kradebml = (kradebml_t *)userdata;

	kradebml->last_cluster_pos = offset;

	printf("Cluster at: %ld\n", offset);

}


static void
log_callback(nestegg * ctx, unsigned int severity, char const * fmt, ...)
{
  va_list ap;
  char const * sev = NULL;


	if (severity == NESTEGG_LOG_DEBUG) {
		 return;
	}	


  switch (severity) {
  case NESTEGG_LOG_DEBUG:
    sev = "debug:   ";
    break;
  case NESTEGG_LOG_WARNING:
    sev = "warning: ";
    break;
  case NESTEGG_LOG_CRITICAL:
    sev = "critical:";
    break;
  default:
    sev = "unknown: ";
  }

  fprintf(stderr, "%p %s ", (void *) ctx, sev);

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fprintf(stderr, "\n");
}




int kradebml_open_output_stream(kradebml_t *kradebml, char *host, int port, char *mount, char *password) {

	kradebml->io.userdata = kradebml;

	kradebml->io.read = kradebml_stream_read;
	kradebml->io.seek = kradebml_stream_seek;
	kradebml->io.tell = kradebml_stream_tell;
	kradebml->io.write = kradebml_stream_write;

	strncpy(kradebml->filename, mount, 512);
	kradebml->server = server_create(host, port, mount, password);
	kradebml_stream_open(kradebml);

	server_build_http_header(kradebml->server);
	
	server_send(kradebml->server, kradebml->server->headers, strlen(kradebml->server->headers));

	printf("Krad MKV Source: Sent Headers: \n%s\n", kradebml->server->headers);
	
	//usleep(1000000);



	return 0;
}

int kradebml_open_output_file(kradebml_t *kradebml, char *filename) {

	kradebml->io.userdata = kradebml;

	kradebml->io.read = kradebml_stdio_read;
	kradebml->io.seek = kradebml_stdio_seek;
	kradebml->io.tell = kradebml_stdio_tell;
	kradebml->io.write = kradebml_stdio_write;

	strncpy(kradebml->filename, filename, 512);
	
	kradebml_stdio_open(kradebml, "w");

	return 0;
}

void kradebml_close_output(kradebml_t *kradebml) {


    if(kradebml->ebml->cluster_open) {
        Ebml_EndSubElement(kradebml->ebml, &kradebml->ebml->startCluster);	
	}


	kradebml_end_segment(kradebml);

	kradebml_write(kradebml);

	//kradebml_close_output(kradebml);
	
	if (kradebml->server) {
		server_destroy(kradebml->server);
		kradebml->server = NULL;
	} else {
		kradebml_stdio_close(kradebml);
	}
	memset(kradebml->filename, 0, sizeof(kradebml->filename));
	
}

int kradebml_write(kradebml_t *kradebml) {

	if (kradebml->ebml->tracks_open == 1) {
		Ebml_EndSubElement(kradebml->ebml, &kradebml->ebml->tracks);
		kradebml->ebml->tracks_open = 0;
	}


	if (kradebml->server) {
		kradebml_stream_write(kradebml->ebml->buffer, kradebml->ebml->buffer_pos, kradebml);
	} else {

		ke_io_write(&kradebml->io, kradebml->ebml->buffer, kradebml->ebml->buffer_pos);

	}

    kradebml->ebml->buffer_pos = 0;

	return 0;

}


void kradebml_header(kradebml_t *kradebml, char *doctype, char *appversion) {
    
    
	EbmlGlobal *glob = kradebml->ebml;
    
	EbmlLoc start;
	Ebml_StartSubElement(glob, &start, EBML);
	Ebml_SerializeUnsigned(glob, EBMLVersion, 1);
	Ebml_SerializeUnsigned(glob, EBMLReadVersion, 1); //EBML Read Version
	Ebml_SerializeUnsigned(glob, EBMLMaxIDLength, 4); //EBML Max ID Length
	Ebml_SerializeUnsigned(glob, EBMLMaxSizeLength, 8); //EBML Max Size Length
	Ebml_SerializeString(glob, DocType, doctype); //Doc Type
	
	if ((strncmp("webm", doctype, 4) == 0) || (strncmp("matroska", doctype, 4) == 0)) {
		Ebml_SerializeUnsigned(glob, DocTypeVersion, 2); //Doc Type Version
		Ebml_SerializeUnsigned(glob, DocTypeReadVersion, 2); //Doc Type Read Version
	} else {
		Ebml_SerializeUnsigned(glob, DocTypeVersion, 1); //Doc Type Version
		Ebml_SerializeUnsigned(glob, DocTypeReadVersion, 1); //Doc Type Read Version
	}
	Ebml_EndSubElement(glob, &start);


	kradebml_start_segment(kradebml, appversion);

}

void kradebml_end_segment(kradebml_t *kradebml) {

	EbmlGlobal *glob = kradebml->ebml;

	Ebml_EndSubElement(glob, &glob->startSegment);
}

void kradebml_start_segment(kradebml_t *kradebml, char *appversion) {

	EbmlGlobal *glob = kradebml->ebml;

	Ebml_StartSubElement(glob, &glob->startSegment, Segment); //segment

	//glob->framerate = *fps;

	EbmlLoc startInfo;
	//uint64_t frame_time;
	char version_string[64];

	strcpy(version_string, "Krad EBML ");

	strcat(version_string, KRADEBML_VERSION);

	//frame_time = (uint64_t)1000 * glob->framerate.den / glob->framerate.num;

	Ebml_StartSubElement(glob, &startInfo, Info);
	Ebml_SerializeUnsigned(glob, TimecodeScale, 1000000);
	//Ebml_SerializeFloat(glob, Segment_Duration, glob->last_pts_ms + frame_time);
	Ebml_SerializeString(glob, 0x4D80, version_string);
	Ebml_SerializeString(glob, 0x5741, appversion);
	Ebml_EndSubElement(glob, &startInfo);

	// kludege possitioning
	//Ebml_StartSubElement(glob, &glob->tracks, Tracks);
	//glob->tracks_open = 1;
	
//    Ebml_EndSubElement(glob, &glob->tracks);

}

int kradebml_new_tracknumber(kradebml_t *kradebml) {

	int current_tracknumber;
	
	current_tracknumber = kradebml->next_tracknumber;

	kradebml->total_tracks++;

	kradebml->next_tracknumber++;
	
	return current_tracknumber;

}

int kradebml_add_video_track_with_priv(kradebml_t *kradebml, char *codec_id, int frame_rate, int width, int height, unsigned char *private_data, int private_data_size) {

	EbmlGlobal *glob = kradebml->ebml;

	if (kradebml->ebml->tracks_open == 0) {
		Ebml_StartSubElement(kradebml->ebml, &kradebml->ebml->tracks, Tracks);
		kradebml->ebml->tracks_open = 1;
	}

	glob->total_video_frames = -1;

	glob->video_frame_rate = frame_rate;


	unsigned int trackNumber = kradebml_new_tracknumber(kradebml);
	//float        frameRate   = (float)fps->num/(float)fps->den;

	EbmlLoc trackstart;
	Ebml_StartSubElement(glob, &trackstart, TrackEntry);
	Ebml_SerializeUnsigned(glob, TrackNumber, trackNumber);
	Ebml_SerializeUnsigned(glob, TrackUID, 1);
	Ebml_SerializeUnsigned(glob, FlagLacing, 0);
	//Ebml_SerializeUnsigned(glob, FlagDefault, 1);
	Ebml_SerializeString(glob, CodecID, codec_id);
	//Ebml_SerializeFloat(glob, FrameRate, frameRate);
	Ebml_SerializeUnsigned(glob, TrackType, 1); //video is always 1

	if (private_data_size > 0) {
		Ebml_SerializeData(glob, CodecPrivate, private_data, private_data_size);
	}
	
	unsigned int pixelWidth = width;
	unsigned int pixelHeight = height;

	EbmlLoc videoStart;
	Ebml_StartSubElement(glob, &videoStart, Video);
	Ebml_SerializeUnsigned(glob, PixelWidth, pixelWidth);
	Ebml_SerializeUnsigned(glob, PixelHeight, pixelHeight);
	//Ebml_SerializeUnsigned(glob, StereoMode, STEREO_FORMAT_MONO);
	Ebml_EndSubElement(glob, &videoStart); //Video

	Ebml_EndSubElement(glob, &trackstart); //Track Entry


}

int kradebml_add_video_track(kradebml_t *kradebml, char *codec_id, int frame_rate, int width, int height) {


	EbmlGlobal *glob = kradebml->ebml;

	if (kradebml->ebml->tracks_open == 0) {
		Ebml_StartSubElement(kradebml->ebml, &kradebml->ebml->tracks, Tracks);
		kradebml->ebml->tracks_open = 1;
	}

	glob->total_video_frames = -1;

	glob->video_frame_rate = frame_rate;


	unsigned int trackNumber = kradebml_new_tracknumber(kradebml);
	//float        frameRate   = (float)fps->num/(float)fps->den;

	EbmlLoc trackstart;
	Ebml_StartSubElement(glob, &trackstart, TrackEntry);
	Ebml_SerializeUnsigned(glob, TrackNumber, trackNumber);
	Ebml_SerializeUnsigned(glob, TrackUID, 1);
	Ebml_SerializeUnsigned(glob, FlagLacing, 0);
	//Ebml_SerializeUnsigned(glob, FlagDefault, 1);
	Ebml_SerializeString(glob, CodecID, codec_id);
	//Ebml_SerializeFloat(glob, FrameRate, frameRate);
	Ebml_SerializeUnsigned(glob, TrackType, 1); //video is always 1

	unsigned int pixelWidth = width;
	unsigned int pixelHeight = height;

	EbmlLoc videoStart;
	Ebml_StartSubElement(glob, &videoStart, Video);
	Ebml_SerializeUnsigned(glob, PixelWidth, pixelWidth);
	Ebml_SerializeUnsigned(glob, PixelHeight, pixelHeight);
	//Ebml_SerializeUnsigned(glob, StereoMode, STEREO_FORMAT_MONO);
	Ebml_EndSubElement(glob, &videoStart); //Video

	Ebml_EndSubElement(glob, &trackstart); //Track Entry
	
	
	
	
	// kludege possitioning
    //    Ebml_EndSubElement(kradebml->ebml, &kradebml->ebml->tracks);	
	
	
	
}

int kradebml_add_subtitle_track(kradebml_t *kradebml, char *codec_id) {

	EbmlGlobal *glob = kradebml->ebml;


	if (kradebml->ebml->tracks_open == 0) {
		Ebml_StartSubElement(kradebml->ebml, &kradebml->ebml->tracks, Tracks);
		kradebml->ebml->tracks_open = 1;
	}


	int tracknumber = kradebml_new_tracknumber(kradebml);

	EbmlLoc track;
	
	
	//kradebml->audio_sample_rate = sample_rate;
	//kradebml->audio_channels = channels;
	
	Ebml_StartSubElement(glob, &track, TrackEntry);
	
	Ebml_SerializeUnsigned(glob, TrackNumber, tracknumber);
	Ebml_SerializeUnsigned(glob, TrackUID, tracknumber);
	
	Ebml_SerializeUnsigned(glob, FlagLacing, 0);
	Ebml_SerializeUnsigned(glob, FlagDefault, 0);


	Ebml_SerializeString(glob, CodecID, codec_id);
	Ebml_SerializeUnsigned(glob, TrackType, 0x11); 

	//EbmlLoc AudioStart;
	//Ebml_StartSubElement(glob, &AudioStart, Audio);
	//Ebml_SerializeUnsigned(glob, Channels, channels);
	//Ebml_SerializeFloat(glob, SamplingFrequency, sample_rate);
	//Ebml_SerializeUnsigned(glob, BitDepth, 16);
	//Ebml_EndSubElement(glob, &AudioStart);


	//if (private_data_size > 0) {

	//	Ebml_SerializeData(glob, CodecPrivate, private_data, private_data_size);

	//}


    Ebml_EndSubElement(glob, &track);
 
	//Ebml_EndSubElement(glob, &glob->tracks);


	return tracknumber;

}

int kradebml_add_audio_track(kradebml_t *kradebml, char *codec_id, float sample_rate, int channels, unsigned char *private_data, int private_data_size) {

	EbmlGlobal *glob = kradebml->ebml;


	if (kradebml->ebml->tracks_open == 0) {
		Ebml_StartSubElement(kradebml->ebml, &kradebml->ebml->tracks, Tracks);
		kradebml->ebml->tracks_open = 1;
	}


	int tracknumber = kradebml_new_tracknumber(kradebml);

	EbmlLoc track;
	
	
	kradebml->audio_sample_rate = sample_rate;
	kradebml->audio_channels = channels;
	
	Ebml_StartSubElement(glob, &track, TrackEntry);
	
	Ebml_SerializeUnsigned(glob, TrackNumber, tracknumber);
	Ebml_SerializeUnsigned(glob, TrackUID, tracknumber);
	
	Ebml_SerializeUnsigned(glob, FlagLacing, 0);
	//Ebml_SerializeUnsigned(glob, FlagDefault, 1);


	Ebml_SerializeString(glob, CodecID, codec_id);
	Ebml_SerializeUnsigned(glob, TrackType, 2); //audio is always 2

	EbmlLoc AudioStart;
	Ebml_StartSubElement(glob, &AudioStart, Audio);
	Ebml_SerializeUnsigned(glob, Channels, channels);
	Ebml_SerializeFloat(glob, SamplingFrequency, sample_rate);
	Ebml_SerializeUnsigned(glob, BitDepth, 16);
	Ebml_EndSubElement(glob, &AudioStart);


	if (private_data_size > 0) {

		Ebml_SerializeData(glob, CodecPrivate, private_data, private_data_size);

	}


    Ebml_EndSubElement(glob, &track);
 
	//Ebml_EndSubElement(glob, &glob->tracks);


	return tracknumber;

}

void kradebml_add_video(kradebml_t *kradebml, int track_num, unsigned char *buffer, int bufferlen, int keyframe)
{

	EbmlGlobal *glob = kradebml->ebml;

    unsigned long block_length;
    unsigned char track_number;
    unsigned short block_timecode = 0;
    unsigned char flags;
    //int64_t pts_ms;


	glob->total_video_frames += 1;
	
	int64_t timecode = glob->total_video_frames * 1000 * (uint64_t)1 / (uint64_t)glob->video_frame_rate;
    //pts_ms = pkt->data.frame.pts * 1000 * (uint64_t)cfg->g_timebase.num / (uint64_t)cfg->g_timebase.den;
	if ((keyframe) && ((glob->total_video_frames > 0) || (kradebml->total_tracks == 1))) {
		kradebml_cluster(kradebml, timecode);
	}

	block_timecode = timecode - glob->cluster_timecode;

	//printf("timecode is %ld and buffer len is %d keyframe is %d\n", timecode, bufferlen, keyframe);
	

    /* Write the Simple Block */
    Ebml_WriteID(glob, SimpleBlock);

    block_length = bufferlen + 4;
    block_length |= 0x10000000;
    Ebml_Serialize(glob, &block_length, sizeof(block_length), 4);

    track_number = 1;
    track_number |= 0x80;
    Ebml_Write(glob, &track_number, 1);

    Ebml_Serialize(glob, &block_timecode, sizeof(block_timecode), 2);
	//printf("video block timecode is: %d\n", block_timecode);
    flags = 0;
    if(keyframe)
        flags |= 0x80;
    //if(pkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE)
    //    flags |= 0x08;
    Ebml_Write(glob, &flags, 1);

    Ebml_Write(glob, buffer, bufferlen);


}

void kradebml_add_audio(kradebml_t *kradebml, int track_num, unsigned char *buffer, int bufferlen, int frames) {

	EbmlGlobal *glob = kradebml->ebml;
	int64_t timecode;
		
	if (kradebml->total_audio_frames > 0) {
		timecode = (kradebml->total_audio_frames)/ kradebml->audio_sample_rate * 1000;
	} else {
		timecode = 0;
	}
	kradebml->total_audio_frames += frames;
	

	if (timecode == 0) {
			kradebml_cluster(kradebml, timecode);
	}	

	
	//int64_t timecode = glob->total_video_frames * 1000 * (uint64_t)1 / (uint64_t)glob->video_frame_rate;
    //pts_ms = pkt->data.frame.pts * 1000 * (uint64_t)cfg->g_timebase.num / (uint64_t)cfg->g_timebase.den;

	printf("timecode is %ld and buffer len is %d\n", timecode, bufferlen);
	
	unsigned long  block_length;
    unsigned char  track_number;
	short block_timecode = 0;
	unsigned char  flags;

	//block_timecode = glob->last_pts_ms - glob->cluster_timecode;
	//block_timecode += 5;
	block_timecode = timecode - glob->cluster_timecode;
	//printf("audio timecode is: %d\n", block_timecode);

    // Write the Simple Block 
    Ebml_WriteID(glob, SimpleBlock);

    block_length = bufferlen + 4;
    block_length |= 0x10000000;
    Ebml_Serialize(glob, &block_length, sizeof(block_length), 4);
    
    track_number = track_num;
    track_number |= 0x80;
    Ebml_Write(glob, &track_number, 1);

	Ebml_Serialize(glob, &block_timecode, sizeof(block_timecode), 2);

	flags = 0;
	flags |= 0x80;
	Ebml_Write(glob, &flags, 1);

    Ebml_Write(glob, buffer, bufferlen);


	//kradebml_end_cluster(kradebml);

}

void kradebml_cluster(kradebml_t *kradebml, int timecode) {

	EbmlGlobal *glob = kradebml->ebml;

    unsigned long block_length;
    unsigned char track_number;
    unsigned short block_timecode = 0;
    unsigned char flags;
    int64_t pts_ms;
    int is_keyframe;

    /* Calculate the PTS of this frame in milliseconds */
    //pts_ms = pkt->data.frame.pts * 1000 * (uint64_t)cfg->g_timebase.num / (uint64_t)cfg->g_timebase.den;
	
	pts_ms = timecode;
	
	glob->last_pts_ms = timecode;

    /* Calculate the relative time of this block */
	block_timecode = pts_ms - glob->cluster_timecode;
	
  //  is_keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY);

//    if (is_keyframe) {

        if(glob->cluster_open) {
            Ebml_EndSubElement(glob, &glob->startCluster);	
		}

        /* Open the new cluster */
        block_timecode = 0;
        glob->cluster_open = 1;
        glob->cluster_timecode = pts_ms;
        //glob->cluster_pos = ftello(glob->stream);
        Ebml_StartSubElement(glob, &glob->startCluster, Cluster); //cluster
		//Ebml_SerializeUnsigned(glob, Position, 0);
        Ebml_SerializeUnsigned(glob, Timecode, glob->cluster_timecode);
	

		//printf("cluster timecode is: %d\n", glob->cluster_timecode);

    //}

    /* Write the Simple Block 
    Ebml_WriteID(glob, SimpleBlock);

    block_length = pkt->data.frame.sz + 4;
    block_length |= 0x10000000;
    Ebml_Serialize(glob, &block_length, sizeof(block_length), 4);

    track_number = 1;
    track_number |= 0x80;
    Ebml_Write(glob, &track_number, 1);

    Ebml_Serialize(glob, &block_timecode, sizeof(block_timecode), 2);
	//printf("video block timecode is: %d\n", block_timecode);
    flags = 0;
    if(is_keyframe)
        flags |= 0x80;
    if(pkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE)
        flags |= 0x08;
    Ebml_Write(glob, &flags, 1);

    Ebml_Write(glob, pkt->data.frame.buf, pkt->data.frame.sz);
	*/
}


void Ebml_Write(EbmlGlobal *glob, const void *buffer_in, unsigned long len)
{
    //if(fwrite(buffer_in, 1, len, glob->stream));
   
    memcpy(glob->buffer + glob->buffer_pos, buffer_in, len);
    glob->buffer_pos += len;
    //kradsource_write(glob->kradsource, buffer_in, len);
    //printf("-------------------- buffer pos is: %lld\n", glob->buffer_pos);
    //kradsource_write(glob->kradsource, glob->buffer, glob->buffer_pos);
    //glob->buffer_pos = 0;
}

#define WRITE_BUFFER(s) \
for(i = len-1; i>=0; i--)\
{ \
    x = *(const s *)buffer_in >> (i * CHAR_BIT); \
    Ebml_Write(glob, &x, 1); \
}
void Ebml_Serialize(EbmlGlobal *glob, const void *buffer_in, int buffer_size, unsigned long len)
{
    char x;
    int i;

    /* buffer_size:
     * 1 - int8_t;
     * 2 - int16_t;
     * 3 - int32_t;
     * 4 - int64_t;
     */
    switch (buffer_size)
    {
        case 1:
            WRITE_BUFFER(int8_t)
            break;
        case 2:
            WRITE_BUFFER(int16_t)
            break;
        case 4:
            WRITE_BUFFER(int32_t)
            break;
        case 8:
            WRITE_BUFFER(int64_t)
            break;
        default:
            break;
    }
}
#undef WRITE_BUFFER

/* Need a fixed size serializer for the track ID. libmkv provides a 64 bit
 * one, but not a 32 bit one.
 */
void Ebml_SerializeUnsigned32(EbmlGlobal *glob, unsigned long class_id, uint64_t ui)
{
    unsigned char sizeSerialized = 4 | 0x80;
    Ebml_WriteID(glob, class_id);
    Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
    Ebml_Serialize(glob, &ui, sizeof(ui), 4);
}


void
Ebml_StartSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc,
                          unsigned long class_id)
{

    uint64_t unknownLen = LITERALU64(0x01FFFFFFFFFFFFFF);

    Ebml_WriteID(glob, class_id);

	*ebmlLoc = glob->buffer_pos;
	
    Ebml_Serialize(glob, &unknownLen, sizeof(unknownLen), 8);
}

void
Ebml_EndSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc)
{
	off_t pos;
	off_t size;
	uint64_t len;

    pos = glob->buffer_pos;

    size = pos - *ebmlLoc - 8;

    len = size;
    
    len |= LITERALU64(0x0100000000000000);

    glob->buffer_pos = *ebmlLoc;

    Ebml_Serialize(glob, &len, sizeof(len), 8);

    glob->buffer_pos = pos;
}

/*
void write_webm_seek_element(EbmlGlobal *ebml, unsigned long id, off_t pos)
{
    uint64_t offset = pos - ebml->position_reference;
    EbmlLoc start;
    Ebml_StartSubElement(ebml, &start, Seek);
    Ebml_SerializeBinary(ebml, SeekID, id);
    Ebml_SerializeUnsigned64(ebml, SeekPosition, offset);
    Ebml_EndSubElement(ebml, &start);
}


void write_webm_info(EbmlGlobal *ebml)
{

	EbmlLoc startInfo;
	uint64_t frame_time;
	char version_string[64];

	strcpy(version_string, "kradcomposite");

	frame_time = (uint64_t)1000 * ebml->framerate.den / ebml->framerate.num;

	Ebml_StartSubElement(ebml, &startInfo, Info);
	Ebml_SerializeUnsigned(ebml, TimecodeScale, 1000000);
	Ebml_SerializeFloat(ebml, Duration, ebml->last_pts_ms + frame_time);
	Ebml_SerializeString(ebml, 0x4D80, version_string);
	Ebml_SerializeString(ebml, 0x5741, version_string);
	Ebml_EndSubElement(ebml, &startInfo);
    
}
*/
void Ebml_SerializeData(EbmlGlobal *glob, unsigned long class_id, unsigned char *data, unsigned long data_length)
{
    Ebml_WriteID(glob, class_id);
    Ebml_WriteLen(glob, data_length);
    Ebml_Write(glob,  data, data_length);
}






void Ebml_WriteID(EbmlGlobal *glob, unsigned long class_id)
{
    int len;

    if (class_id >= 0x01000000)
        len = 4;
    else if (class_id >= 0x00010000)
        len = 3;
    else if (class_id >= 0x00000100)
        len = 2;
    else
        len = 1;

    Ebml_Serialize(glob, (void *)&class_id, sizeof(class_id), len);
}

void Ebml_SerializeUnsigned64(EbmlGlobal *glob, unsigned long class_id, uint64_t ui)
{
    unsigned char sizeSerialized = 8 | 0x80;
    Ebml_WriteID(glob, class_id);
    Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
    Ebml_Serialize(glob, &ui, sizeof(ui), 8);
}

void Ebml_SerializeUnsigned(EbmlGlobal *glob, unsigned long class_id, unsigned long ui)
{
    unsigned char size = 8; //size in bytes to output
    unsigned char sizeSerialized = 0;
    unsigned long minVal;

    Ebml_WriteID(glob, class_id);
    minVal = 0x7fLU; //mask to compare for byte size

    for (size = 1; size < 4; size ++)
    {
        if (ui < minVal)
        {
            break;
        }

        minVal <<= 7;
    }

    sizeSerialized = 0x80 | size;
    Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
    Ebml_Serialize(glob, &ui, sizeof(ui), size);
}
//TODO: perhaps this is a poor name for this id serializer helper function
void Ebml_SerializeBinary(EbmlGlobal *glob, unsigned long class_id, unsigned long bin)
{
    int size;
    for (size=4; size > 1; size--)
    {
        if (bin & 0x000000ff << ((size-1) * 8))
            break;
    }
    Ebml_WriteID(glob, class_id);
    Ebml_WriteLen(glob, size);
    Ebml_WriteID(glob, bin);
}

void Ebml_SerializeFloat(EbmlGlobal *glob, unsigned long class_id, double d)
{
    unsigned char len = 0x88;

    Ebml_WriteID(glob, class_id);
    Ebml_Serialize(glob, &len, sizeof(len), 1);
    Ebml_Serialize(glob,  &d, sizeof(d), 8);
}


void Ebml_WriteLen(EbmlGlobal *glob, long long val)
{
    //TODO check and make sure we are not > than 0x0100000000000000LLU
    unsigned char size = 8; //size in bytes to output
    unsigned long long minVal = LITERALU64(0x00000000000000ff); //mask to compare for byte size

    for (size = 1; size < 8; size ++)
    {
        if (val < minVal)
            break;

        minVal = (minVal << 7);
    }

    val |= (LITERALU64(0x000000000000080) << ((size - 1) * 7));

    Ebml_Serialize(glob, (void *) &val, sizeof(val), size);
}

void Ebml_SerializeString(EbmlGlobal *glob, unsigned long class_id, const char *s)
{
    Ebml_WriteID(glob, class_id);
    Ebml_WriteString(glob, s);
}

void Ebml_WriteString(EbmlGlobal *glob, const char *str)
{
    const size_t size_ = strlen(str);
    const unsigned long long  size = size_;
    Ebml_WriteLen(glob, size);
    //TODO: it's not clear from the spec whether the nul terminator
    //should be serialized too.  For now we omit the null terminator.
    Ebml_Write(glob, str, size);
}







//////////////////////////////////////

/* EBML Elements */
#define ID_EBML                 0x1a45dfa3
#define ID_EBML_VERSION         0x4286
#define ID_EBML_READ_VERSION    0x42f7
#define ID_EBML_MAX_ID_LENGTH   0x42f2
#define ID_EBML_MAX_SIZE_LENGTH 0x42f3
#define ID_DOCTYPE              0x4282
#define ID_DOCTYPE_VERSION      0x4287
#define ID_DOCTYPE_READ_VERSION 0x4285

/* Global Elements */
#define ID_VOID                 0xec
#define ID_CRC32                0xbf

/* WebM Elements */
#define ID_SEGMENT              0x18538067

/* Seek Head Elements */
#define ID_SEEK_HEAD            0x114d9b74
#define ID_SEEK                 0x4dbb
#define ID_SEEK_ID              0x53ab
#define ID_SEEK_POSITION        0x53ac

/* Info Elements */
#define ID_INFO                 0x1549a966
#define ID_TIMECODE_SCALE       0x2ad7b1
#define ID_DURATION             0x4489

/* Cluster Elements */
#define ID_CLUSTER              0x1f43b675
#define ID_TIMECODE             0xe7
#define ID_BLOCK_GROUP          0xa0
#define ID_SIMPLE_BLOCK         0xa3

/* BlockGroup Elements */
#define ID_BLOCK                0xa1
#define ID_BLOCK_DURATION       0x9b
#define ID_REFERENCE_BLOCK      0xfb

/* Tracks Elements */
#define ID_TRACKS               0x1654ae6b
#define ID_TRACK_ENTRY          0xae
#define ID_TRACK_NUMBER         0xd7
#define ID_TRACK_UID            0x73c5
#define ID_TRACK_TYPE           0x83
#define ID_FLAG_ENABLED         0xb9
#define ID_FLAG_DEFAULT         0x88
#define ID_FLAG_LACING          0x9c
#define ID_TRACK_TIMECODE_SCALE 0x23314f
#define ID_LANGUAGE             0x22b59c
#define ID_CODEC_ID             0x86
#define ID_CODEC_PRIVATE        0x63a2

/* Video Elements */
#define ID_VIDEO                0xe0
#define ID_STEREO_MODE          0x53b8
#define ID_PIXEL_WIDTH          0xb0
#define ID_PIXEL_HEIGHT         0xba
#define ID_PIXEL_CROP_BOTTOM    0x54aa
#define ID_PIXEL_CROP_TOP       0x54bb
#define ID_PIXEL_CROP_LEFT      0x54cc
#define ID_PIXEL_CROP_RIGHT     0x54dd
#define ID_DISPLAY_WIDTH        0x54b0
#define ID_DISPLAY_HEIGHT       0x54ba

/* Audio Elements */
#define ID_AUDIO                0xe1
#define ID_SAMPLING_FREQUENCY   0xb5
#define ID_CHANNELS             0x9f
#define ID_BIT_DEPTH            0x6264

/* Cues Elements */
#define ID_CUES                 0x1c53bb6b
#define ID_CUE_POINT            0xbb
#define ID_CUE_TIME             0xb3
#define ID_CUE_TRACK_POSITIONS  0xb7
#define ID_CUE_TRACK            0xf7
#define ID_CUE_CLUSTER_POSITION 0xf1
#define ID_CUE_BLOCK_NUMBER     0x5378

/* EBML Types */
enum ebml_type_enum {
  TYPE_UNKNOWN,
  TYPE_MASTER,
  TYPE_UINT,
  TYPE_FLOAT,
  TYPE_INT,
  TYPE_STRING,
  TYPE_BINARY
};

#define LIMIT_STRING            (1 << 20)
#define LIMIT_BINARY            (1 << 24)
#define LIMIT_BLOCK             (1 << 30)
#define LIMIT_FRAME             (1 << 28)

/* Field Flags */
#define DESC_FLAG_NONE          0
#define DESC_FLAG_MULTI         (1 << 0)
#define DESC_FLAG_SUSPEND       (1 << 1)
#define DESC_FLAG_OFFSET        (1 << 2)

/* Block Header Flags */
#define BLOCK_FLAGS_LACING      6

/* Lacing Constants */
#define LACING_NONE             0
#define LACING_XIPH             1
#define LACING_FIXED            2
#define LACING_EBML             3

/* Track Types */
#define TRACK_TYPE_VIDEO        1
#define TRACK_TYPE_AUDIO        2
#define TRACK_TYPE_SUBTITLE		3

/* Track IDs */
#define TRACK_ID_VP8            "V_VP8"
#define TRACK_ID_VORBIS         "A_VORBIS"
#define TRACK_ID_OPUS        	"A_KRAD_OPUS"
#define TRACK_ID_FLAC         	"A_FLAC"
#define TRACK_ID_DIRAC         	"V_DIRAC"
#define TRACK_ID_THEORA         "V_THEORA"

enum vint_mask {
  MASK_NONE,
  MASK_FIRST_BIT
};

struct ebml_binary {
  unsigned char * data;
  size_t length;
};

struct ebml_list_node {
  struct ebml_list_node * next;
  uint64_t id;
  void * data;
};

struct ebml_list {
  struct ebml_list_node * head;
  struct ebml_list_node * tail;
};

struct ebml_type {
  union ebml_value {
    uint64_t u;
    double f;
    int64_t i;
    char * s;
    struct ebml_binary b;
  } v;
  enum ebml_type_enum type;
  int read;
};

/* EBML Definitions */
struct ebml {
  struct ebml_type ebml_version;
  struct ebml_type ebml_read_version;
  struct ebml_type ebml_max_id_length;
  struct ebml_type ebml_max_size_length;
  struct ebml_type doctype;
  struct ebml_type doctype_version;
  struct ebml_type doctype_read_version;
};

/* Matroksa Definitions */
struct seek {
  struct ebml_type id;
  struct ebml_type position;
};

struct seek_head {
  struct ebml_list seek;
};

struct info {
  struct ebml_type timecode_scale;
  struct ebml_type duration;
};

struct block_group {
  struct ebml_type duration;
  struct ebml_type reference_block;
};

struct cluster {
  struct ebml_type timecode;
  struct ebml_list block_group;
};

struct video {
  struct ebml_type stereo_mode;
  struct ebml_type pixel_width;
  struct ebml_type pixel_height;
  struct ebml_type pixel_crop_bottom;
  struct ebml_type pixel_crop_top;
  struct ebml_type pixel_crop_left;
  struct ebml_type pixel_crop_right;
  struct ebml_type display_width;
  struct ebml_type display_height;
};

struct audio {
  struct ebml_type sampling_frequency;
  struct ebml_type channels;
  struct ebml_type bit_depth;
};

struct track_entry {
  struct ebml_type number;
  struct ebml_type uid;
  struct ebml_type type;
  struct ebml_type flag_enabled;
  struct ebml_type flag_default;
  struct ebml_type flag_lacing;
  struct ebml_type track_timecode_scale;
  struct ebml_type language;
  struct ebml_type codec_id;
  struct ebml_type codec_private;
  struct video video;
  struct audio audio;
};

struct tracks {
  struct ebml_list track_entry;
};

struct cue_track_positions {
  struct ebml_type track;
  struct ebml_type cluster_position;
  struct ebml_type block_number;
};

struct cue_point {
  struct ebml_type time;
  struct ebml_list cue_track_positions;
};

struct cues {
  struct ebml_list cue_point;
};

struct segment {
  struct ebml_list seek_head;
  struct info info;
  struct ebml_list cluster;
  struct tracks tracks;
  struct cues cues;
};

/* Misc. */
struct pool_ctx {
  char dummy;
};

struct list_node {
  struct list_node * previous;
  struct ebml_element_desc * node;
  unsigned char * data;
};

struct saved_state {
  int64_t stream_offset;
  struct list_node * ancestor;
  uint64_t last_id;
  uint64_t last_size;
};

struct frame {
  unsigned char * data;
  size_t length;
  struct frame * next;
};

/* Public (opaque) Structures */
struct nestegg {
  nestegg_io * io;
  nestegg_log log;
  nestegg_info *info;
  struct pool_ctx * alloc_pool;
  uint64_t last_id;
  uint64_t last_size;
  struct list_node * ancestor;
  struct ebml ebml;
  struct segment segment;
  int64_t segment_offset;
  unsigned int track_count;
  
 int64_t tracks_start;
 int64_t tracks_end;
 size_t tracks_length;
 char tracks[8192 * 2];
  
};

struct nestegg_packet {
  uint64_t track;
  uint64_t timecode;
  struct frame * frame;
};

/* Element Descriptor */
struct ebml_element_desc {
  char const * name;
  uint64_t id;
  enum ebml_type_enum type;
  size_t offset;
  unsigned int flags;
  struct ebml_element_desc * children;
  size_t size;
  size_t data_offset;
};

#define E_FIELD(ID, TYPE, STRUCT, FIELD) \
  { #ID, ID, TYPE, offsetof(STRUCT, FIELD), DESC_FLAG_NONE, NULL, 0, 0 }
#define E_MASTER(ID, TYPE, STRUCT, FIELD) \
  { #ID, ID, TYPE, offsetof(STRUCT, FIELD), DESC_FLAG_MULTI, ne_ ## FIELD ## _elements, \
      sizeof(struct FIELD), 0 }
#define E_SINGLE_MASTER_O(ID, TYPE, STRUCT, FIELD) \
  { #ID, ID, TYPE, offsetof(STRUCT, FIELD), DESC_FLAG_OFFSET, ne_ ## FIELD ## _elements, 0, \
      offsetof(STRUCT, FIELD ## _offset) }
#define E_SINGLE_MASTER(ID, TYPE, STRUCT, FIELD) \
  { #ID, ID, TYPE, offsetof(STRUCT, FIELD), DESC_FLAG_NONE, ne_ ## FIELD ## _elements, 0, 0 }
#define E_SUSPEND(ID, TYPE) \
  { #ID, ID, TYPE, 0, DESC_FLAG_SUSPEND, NULL, 0, 0 }
#define E_LAST \
  { NULL, 0, 0, 0, DESC_FLAG_NONE, NULL, 0, 0 }

/* EBML Element Lists */
static struct ebml_element_desc ne_ebml_elements[] = {
  E_FIELD(ID_EBML_VERSION, TYPE_UINT, struct ebml, ebml_version),
  E_FIELD(ID_EBML_READ_VERSION, TYPE_UINT, struct ebml, ebml_read_version),
  E_FIELD(ID_EBML_MAX_ID_LENGTH, TYPE_UINT, struct ebml, ebml_max_id_length),
  E_FIELD(ID_EBML_MAX_SIZE_LENGTH, TYPE_UINT, struct ebml, ebml_max_size_length),
  E_FIELD(ID_DOCTYPE, TYPE_STRING, struct ebml, doctype),
  E_FIELD(ID_DOCTYPE_VERSION, TYPE_UINT, struct ebml, doctype_version),
  E_FIELD(ID_DOCTYPE_READ_VERSION, TYPE_UINT, struct ebml, doctype_read_version),
  E_LAST
};

/* WebM Element Lists */
static struct ebml_element_desc ne_seek_elements[] = {
  E_FIELD(ID_SEEK_ID, TYPE_BINARY, struct seek, id),
  E_FIELD(ID_SEEK_POSITION, TYPE_UINT, struct seek, position),
  E_LAST
};

static struct ebml_element_desc ne_seek_head_elements[] = {
  E_MASTER(ID_SEEK, TYPE_MASTER, struct seek_head, seek),
  E_LAST
};

static struct ebml_element_desc ne_info_elements[] = {
  E_FIELD(ID_TIMECODE_SCALE, TYPE_UINT, struct info, timecode_scale),
  E_FIELD(ID_DURATION, TYPE_FLOAT, struct info, duration),
  E_LAST
};

static struct ebml_element_desc ne_block_group_elements[] = {
  E_SUSPEND(ID_BLOCK, TYPE_BINARY),
  E_FIELD(ID_BLOCK_DURATION, TYPE_UINT, struct block_group, duration),
  E_FIELD(ID_REFERENCE_BLOCK, TYPE_INT, struct block_group, reference_block),
  E_LAST
};

static struct ebml_element_desc ne_cluster_elements[] = {
  E_FIELD(ID_TIMECODE, TYPE_UINT, struct cluster, timecode),
  E_MASTER(ID_BLOCK_GROUP, TYPE_MASTER, struct cluster, block_group),
  E_SUSPEND(ID_SIMPLE_BLOCK, TYPE_BINARY),
  E_LAST
};

static struct ebml_element_desc ne_video_elements[] = {
  E_FIELD(ID_STEREO_MODE, TYPE_UINT, struct video, stereo_mode),
  E_FIELD(ID_PIXEL_WIDTH, TYPE_UINT, struct video, pixel_width),
  E_FIELD(ID_PIXEL_HEIGHT, TYPE_UINT, struct video, pixel_height),
  E_FIELD(ID_PIXEL_CROP_BOTTOM, TYPE_UINT, struct video, pixel_crop_bottom),
  E_FIELD(ID_PIXEL_CROP_TOP, TYPE_UINT, struct video, pixel_crop_top),
  E_FIELD(ID_PIXEL_CROP_LEFT, TYPE_UINT, struct video, pixel_crop_left),
  E_FIELD(ID_PIXEL_CROP_RIGHT, TYPE_UINT, struct video, pixel_crop_right),
  E_FIELD(ID_DISPLAY_WIDTH, TYPE_UINT, struct video, display_width),
  E_FIELD(ID_DISPLAY_HEIGHT, TYPE_UINT, struct video, display_height),
  E_LAST
};

static struct ebml_element_desc ne_audio_elements[] = {
  E_FIELD(ID_SAMPLING_FREQUENCY, TYPE_FLOAT, struct audio, sampling_frequency),
  E_FIELD(ID_CHANNELS, TYPE_UINT, struct audio, channels),
  E_FIELD(ID_BIT_DEPTH, TYPE_UINT, struct audio, bit_depth),
  E_LAST
};

static struct ebml_element_desc ne_track_entry_elements[] = {
  E_FIELD(ID_TRACK_NUMBER, TYPE_UINT, struct track_entry, number),
  E_FIELD(ID_TRACK_UID, TYPE_UINT, struct track_entry, uid),
  E_FIELD(ID_TRACK_TYPE, TYPE_UINT, struct track_entry, type),
  E_FIELD(ID_FLAG_ENABLED, TYPE_UINT, struct track_entry, flag_enabled),
  E_FIELD(ID_FLAG_DEFAULT, TYPE_UINT, struct track_entry, flag_default),
  E_FIELD(ID_FLAG_LACING, TYPE_UINT, struct track_entry, flag_lacing),
  E_FIELD(ID_TRACK_TIMECODE_SCALE, TYPE_FLOAT, struct track_entry, track_timecode_scale),
  E_FIELD(ID_LANGUAGE, TYPE_STRING, struct track_entry, language),
  E_FIELD(ID_CODEC_ID, TYPE_STRING, struct track_entry, codec_id),
  E_FIELD(ID_CODEC_PRIVATE, TYPE_BINARY, struct track_entry, codec_private),
  E_SINGLE_MASTER(ID_VIDEO, TYPE_MASTER, struct track_entry, video),
  E_SINGLE_MASTER(ID_AUDIO, TYPE_MASTER, struct track_entry, audio),
  E_LAST
};

static struct ebml_element_desc ne_tracks_elements[] = {
  E_MASTER(ID_TRACK_ENTRY, TYPE_MASTER, struct tracks, track_entry),
  E_LAST
};

static struct ebml_element_desc ne_cue_track_positions_elements[] = {
  E_FIELD(ID_CUE_TRACK, TYPE_UINT, struct cue_track_positions, track),
  E_FIELD(ID_CUE_CLUSTER_POSITION, TYPE_UINT, struct cue_track_positions, cluster_position),
  E_FIELD(ID_CUE_BLOCK_NUMBER, TYPE_UINT, struct cue_track_positions, block_number),
  E_LAST
};

static struct ebml_element_desc ne_cue_point_elements[] = {
  E_FIELD(ID_CUE_TIME, TYPE_UINT, struct cue_point, time),
  E_MASTER(ID_CUE_TRACK_POSITIONS, TYPE_MASTER, struct cue_point, cue_track_positions),
  E_LAST
};

static struct ebml_element_desc ne_cues_elements[] = {
  E_MASTER(ID_CUE_POINT, TYPE_MASTER, struct cues, cue_point),
  E_LAST
};

static struct ebml_element_desc ne_segment_elements[] = {
  E_MASTER(ID_SEEK_HEAD, TYPE_MASTER, struct segment, seek_head),
  E_SINGLE_MASTER(ID_INFO, TYPE_MASTER, struct segment, info),
  E_MASTER(ID_CLUSTER, TYPE_MASTER, struct segment, cluster),
  E_SINGLE_MASTER(ID_TRACKS, TYPE_MASTER, struct segment, tracks),
  E_SINGLE_MASTER(ID_CUES, TYPE_MASTER, struct segment, cues),
  E_LAST
};

static struct ebml_element_desc ne_top_level_elements[] = {
  E_SINGLE_MASTER(ID_EBML, TYPE_MASTER, nestegg, ebml),
  E_SINGLE_MASTER_O(ID_SEGMENT, TYPE_MASTER, nestegg, segment),
  E_LAST
};

#undef E_FIELD
#undef E_MASTER
#undef E_SINGLE_MASTER_O
#undef E_SINGLE_MASTER
#undef E_SUSPEND
#undef E_LAST

static struct pool_ctx *
ne_pool_init(void)
{
  struct pool_ctx * pool;

  pool = h_malloc(sizeof(*pool));
  if (!pool)
    abort();
  return pool;
}

static void
ne_pool_destroy(struct pool_ctx * pool)
{
  h_free(pool);
}

static void *
ne_pool_alloc(size_t size, struct pool_ctx * pool)
{
  void * p;

  p = h_malloc(size);
  if (!p)
    abort();
  hattach(p, pool);
  memset(p, 0, size);
  return p;
}

static void *
ne_alloc(size_t size)
{
  void * p;

  p = calloc(1, size);
  if (!p)
    abort();
  return p;
}

static int
ke_io_write(nestegg_io * io, void * buffer, size_t length)
{
  return io->write(buffer, length, io->userdata);
}

static int
ne_io_write(nestegg_io * io, void * buffer, size_t length)
{
  return io->write(buffer, length, io->userdata);
}

static int
ne_io_read(nestegg_io * io, void * buffer, size_t length)
{
  return io->read(buffer, length, io->userdata);
}

static int
ne_io_seek(nestegg_io * io, int64_t offset, int whence)
{
  return io->seek(offset, whence, io->userdata);
}

static int
ne_io_read_skip(nestegg_io * io, size_t length)
{
  size_t get;
  unsigned char buf[8192];
  int r = 1;

  while (length > 0) {
    get = length < sizeof(buf) ? length : sizeof(buf);
    r = ne_io_read(io, buf, get);
    if (r != 1)
      break;
    length -= get;
  }

  return r;
}

static int64_t
ne_io_tell(nestegg_io * io)
{
  return io->tell(io->userdata);
}

static int
ne_bare_read_vint(nestegg_io * io, uint64_t * value, uint64_t * length, enum vint_mask maskflag)
{
  int r;
  unsigned char b;
  size_t maxlen = 8;
  unsigned int count = 1, mask = 1 << 7;

  r = ne_io_read(io, &b, 1);
  if (r != 1)
    return r;

  while (count < maxlen) {
    if ((b & mask) != 0)
      break;
    mask >>= 1;
    count += 1;
  }

  if (length)
    *length = count;
  *value = b;

  if (maskflag == MASK_FIRST_BIT)
    *value = b & ~mask;

  while (--count) {
    r = ne_io_read(io, &b, 1);
    if (r != 1)
      return r;
    *value <<= 8;
    *value |= b;
  }

  return 1;
}

static int
ne_read_id(nestegg_io * io, uint64_t * value, uint64_t * length)
{
  return ne_bare_read_vint(io, value, length, MASK_NONE);
}

static int
ne_read_vint(nestegg_io * io, uint64_t * value, uint64_t * length)
{
  return ne_bare_read_vint(io, value, length, MASK_FIRST_BIT);
}

static int
ne_read_svint(nestegg_io * io, int64_t * value, uint64_t * length)
{
  int r;
  uint64_t uvalue;
  uint64_t ulength;
  int64_t svint_subtr[] = {
    0x3f, 0x1fff,
    0xfffff, 0x7ffffff,
    0x3ffffffffLL, 0x1ffffffffffLL,
    0xffffffffffffLL, 0x7fffffffffffffLL
  };

  r = ne_bare_read_vint(io, &uvalue, &ulength, MASK_FIRST_BIT);
  if (r != 1)
    return r;
  *value = uvalue - svint_subtr[ulength - 1];
  if (length)
    *length = ulength;
  return r;
}

static int
ne_read_uint(nestegg_io * io, uint64_t * val, uint64_t length)
{
  unsigned char b;
  int r;

  if (length == 0 || length > 8)
    return -1;
  r = ne_io_read(io, &b, 1);
  if (r != 1)
    return r;
  *val = b;
  while (--length) {
    r = ne_io_read(io, &b, 1);
    if (r != 1)
      return r;
    *val <<= 8;
    *val |= b;
  }
  return 1;
}

static int
ne_read_int(nestegg_io * io, int64_t * val, uint64_t length)
{
  int r;
  uint64_t uval, base;

  r = ne_read_uint(io, &uval, length);
  if (r != 1)
    return r;

  if (length < sizeof(int64_t)) {
    base = 1;
    base <<= length * 8 - 1;
    if (uval >= base) {
        base = 1;
        base <<= length * 8;
    } else {
      base = 0;
    }
    *val = uval - base;
  } else {
    *val = (int64_t) uval;
  }

  return 1;
}

static int
ne_read_float(nestegg_io * io, double * val, uint64_t length)
{
  union {
    uint64_t u;
    float f;
    double d;
  } value;
  int r;

  /* Length == 10 not implemented. */
  if (length != 4 && length != 8)
    return -1;
  r = ne_read_uint(io, &value.u, length);
  if (r != 1)
    return r;
  if (length == 4)
    *val = value.f;
  else
    *val = value.d;
  return 1;
}

static int
ne_read_string(nestegg * ctx, char ** val, uint64_t length)
{
  char * str;
  int r;

  if (length == 0 || length > LIMIT_STRING)
    return -1;
  str = ne_pool_alloc(length + 1, ctx->alloc_pool);
  r = ne_io_read(ctx->io, (unsigned char *) str, length);
  if (r != 1)
    return r;
  str[length] = '\0';
  *val = str;
  return 1;
}

static int
ne_read_binary(nestegg * ctx, struct ebml_binary * val, uint64_t length)
{
  if (length == 0 || length > LIMIT_BINARY)
    return -1;
  val->data = ne_pool_alloc(length, ctx->alloc_pool);
  val->length = length;
  return ne_io_read(ctx->io, val->data, length);
}

static int
ne_get_uint(struct ebml_type type, uint64_t * value)
{
  if (!type.read)
    return -1;

  assert(type.type == TYPE_UINT);

  *value = type.v.u;

  return 0;
}

static int
ne_get_float(struct ebml_type type, double * value)
{
  if (!type.read)
    return -1;

  assert(type.type == TYPE_FLOAT);

  *value = type.v.f;

  return 0;
}

static int
ne_get_string(struct ebml_type type, char ** value)
{
  if (!type.read)
    return -1;

  assert(type.type == TYPE_STRING);

  *value = type.v.s;

  return 0;
}

static int
ne_get_binary(struct ebml_type type, struct ebml_binary * value)
{
  if (!type.read)
    return -1;

  assert(type.type == TYPE_BINARY);

  *value = type.v.b;

  return 0;
}

static int
ne_is_ancestor_element(uint64_t id, struct list_node * ancestor)
{
  struct ebml_element_desc * element;

  for (; ancestor; ancestor = ancestor->previous)
    for (element = ancestor->node; element->id; ++element)
      if (element->id == id)
        return 1;

  return 0;
}

static struct ebml_element_desc *
ne_find_element(uint64_t id, struct ebml_element_desc * elements)
{
  struct ebml_element_desc * element;

  for (element = elements; element->id; ++element)
    if (element->id == id)
      return element;

  return NULL;
}

static void
ne_ctx_push(nestegg * ctx, struct ebml_element_desc * ancestor, void * data)
{
  struct list_node * item;

  item = ne_alloc(sizeof(*item));
  item->previous = ctx->ancestor;
  item->node = ancestor;
  item->data = data;
  ctx->ancestor = item;
}

static void
ne_ctx_pop(nestegg * ctx)
{
  struct list_node * item;

  item = ctx->ancestor;
  ctx->ancestor = item->previous;
  free(item);
}

static int
ne_ctx_save(nestegg * ctx, struct saved_state * s)
{
  s->stream_offset = ne_io_tell(ctx->io);
  if (s->stream_offset < 0)
    return -1;
  s->ancestor = ctx->ancestor;
  s->last_id = ctx->last_id;
  s->last_size = ctx->last_size;
  return 0;
}

static int
ne_ctx_restore(nestegg * ctx, struct saved_state * s)
{
  int r;

  r = ne_io_seek(ctx->io, s->stream_offset, NESTEGG_SEEK_SET);
  if (r != 0)
    return -1;
  ctx->ancestor = s->ancestor;
  ctx->last_id = s->last_id;
  ctx->last_size = s->last_size;
  return 0;
}

static int
ne_peek_element(nestegg * ctx, uint64_t * id, uint64_t * size)
{
  int r;

  if (ctx->last_id && ctx->last_size) {
    if (id)
      *id = ctx->last_id;
    if (size)
      *size = ctx->last_size;
    return 1;
  }

  r = ne_read_id(ctx->io, &ctx->last_id, NULL);
  if (r != 1)
    return r;

  r = ne_read_vint(ctx->io, &ctx->last_size, NULL);
  if (r != 1)
    return r;

  if (id)
    *id = ctx->last_id;
  if (size)
    *size = ctx->last_size;

  return 1;
}

static int
ne_read_element(nestegg * ctx, uint64_t * id, uint64_t * size)
{
  int r;

  r = ne_peek_element(ctx, id, size);
  if (r != 1)
    return r;

  ctx->last_id = 0;
  ctx->last_size = 0;

  return 1;
}

static void
ne_read_master(nestegg * ctx, struct ebml_element_desc * desc)
{
  struct ebml_list * list;
  struct ebml_list_node * node, * oldtail;

	int64_t cluster_loc;

  assert(desc->type == TYPE_MASTER && desc->flags & DESC_FLAG_MULTI);

  ctx->log(ctx, NESTEGG_LOG_DEBUG, "multi master element %llx (%s)",
           desc->id, desc->name);
           
    if (ctx->info != NULL) {
		if (desc->id == 0x1f43b675) {
			cluster_loc = ne_io_tell(ctx->io);
	  		//ctx->log(ctx, NESTEGG_LOG_WARNING, "clusterbunny multi master element %llx (%s) size %zu", desc->id, desc->name, desc->size);	
			//ctx->info->cluster_at(cluster_loc - 42, ctx->info->userdata);
		}
           
    }
/*
	if (desc->id == 0x1f43b675) {
		cluster_loc = ne_io_tell(ctx->io);
  		ctx->log(ctx, NESTEGG_LOG_DEBUG, "multi master element %llx (%s) size %zu", desc->id, desc->name, desc->size);	
		ctx->info->cluster_at(cluster_loc - 12, ctx->info->userdata);
	} else {
	
    	int64_t segment_offset;
    	segment_offset = ne_io_tell(ctx->io);
	  	printf("azhah sumtin is found!!!!!!!!!!!!!!!! offset: %zu size: %zu filepos: %zu\n", desc->offset, desc->size, segment_offset);

	
	}


	size_t curpos;

	  if ((ctx->tracks_start) && (!(ctx->tracks_end))) {
	  if (desc->id != ID_TRACK_ENTRY) {
		curpos = ne_io_tell(ctx->io);
		ctx->tracks_end = curpos - 12;
		
		ctx->tracks_length = ctx->tracks_end - ctx->tracks_start;
				curpos = ne_io_tell(ctx->io);
		ne_io_seek(ctx->io, ctx->tracks_start, NESTEGG_SEEK_SET);		
		ne_io_read(ctx->io, ctx->tracks, ctx->tracks_length);
		ne_io_seek(ctx->io, curpos - 12, NESTEGG_SEEK_SET);
		ne_io_read_skip(ctx->io, 12);
	  	printf("ahah tracks end found!!!!!!!tracks len is %zu offset: %zu size: %zu filepos s: %zu filepos e: %zu\n", ctx->tracks_length, desc->offset, desc->size, ctx->tracks_start, ctx->tracks_end);
  	}
  }
*/



  list = (struct ebml_list *) (ctx->ancestor->data + desc->offset);

  node = ne_pool_alloc(sizeof(*node), ctx->alloc_pool);
  node->id = desc->id;
  node->data = ne_pool_alloc(desc->size, ctx->alloc_pool);

  oldtail = list->tail;
  if (oldtail)
    oldtail->next = node;
  list->tail = node;
  if (!list->head)
    list->head = node;

  ctx->log(ctx, NESTEGG_LOG_DEBUG, " -> using data %p", node->data);

  ne_ctx_push(ctx, desc->children, node->data);
}

static void
ne_read_single_master(nestegg * ctx, struct ebml_element_desc * desc)
{
  assert(desc->type == TYPE_MASTER && !(desc->flags & DESC_FLAG_MULTI));

  ctx->log(ctx, NESTEGG_LOG_DEBUG, "single master element %llx (%s)",
           desc->id, desc->name);
  ctx->log(ctx, NESTEGG_LOG_DEBUG, " -> using data %p (%u)",
           ctx->ancestor->data + desc->offset, desc->offset);

/*
	int64_t segment_offset;
  //if (desc->id == ID_SEGMENT) {

    	segment_offset = ne_io_tell(ctx->io);
	  	printf("ahah sumtin is found!!!!!!!!!!!!!!!! do %zu offset: %zu size: %zu filepos: %zu\n", desc->data_offset, desc->offset, desc->size, segment_offset);
		//exit(1);
  //}
  
  if (desc->id == ID_TRACKS) {

    	ctx->tracks_start = ne_io_tell(ctx->io) - 6 ;
	  	printf("ahah tracks is found!!!!!!!!!!!!!!!! offset: %zu size: %zu filepos: %zu\n", desc->offset, desc->size, ctx->tracks_start);
  
  }
*/  

  ne_ctx_push(ctx, desc->children, ctx->ancestor->data + desc->offset);
}

static int
ne_read_simple(nestegg * ctx, struct ebml_element_desc * desc, size_t length)
{
  struct ebml_type * storage;
  int r;

  storage = (struct ebml_type *) (ctx->ancestor->data + desc->offset);

  if (storage->read) {
    ctx->log(ctx, NESTEGG_LOG_DEBUG, "element %llx (%s) already read, skipping",
             desc->id, desc->name);
    return 0;
  }

  storage->type = desc->type;

  ctx->log(ctx, NESTEGG_LOG_DEBUG, "element %llx (%s) -> %p (%u)",
           desc->id, desc->name, storage, desc->offset);

  if (desc->id == ID_DURATION) {
	  	//printf("ahah duration is found!!!!!!!!!!!!!!!! L: %zu \n", length);
		//exit(1);
  }


  r = -1;

  switch (desc->type) {
  case TYPE_UINT:
    r = ne_read_uint(ctx->io, &storage->v.u, length);
    break;
  case TYPE_FLOAT:
    r = ne_read_float(ctx->io, &storage->v.f, length);
    break;
  case TYPE_INT:
    r = ne_read_int(ctx->io, &storage->v.i, length);
    break;
  case TYPE_STRING:
    r = ne_read_string(ctx, &storage->v.s, length);
    break;
  case TYPE_BINARY:
    r = ne_read_binary(ctx, &storage->v.b, length);
    break;
  case TYPE_MASTER:
  case TYPE_UNKNOWN:
    assert(0);
    break;
  }

  if (r == 1)
    storage->read = 1;

  return r;
}

static int
ne_parse(nestegg * ctx, struct ebml_element_desc * top_level)
{
  int r;
  int64_t * data_offset;

  int64_t data_offset2;  
  
  uint64_t id, size;
  struct ebml_element_desc * element;

  if (!ctx->ancestor)
    return -1;

  for (;;) {
    r = ne_peek_element(ctx, &id, &size);
    if (r != 1)
      break;

    element = ne_find_element(id, ctx->ancestor->node);
    if (element) {
      if (element->flags & DESC_FLAG_SUSPEND) {
        assert(element->type == TYPE_BINARY);
        ctx->log(ctx, NESTEGG_LOG_DEBUG, "suspend parse at %llx", id);
        r = 1;
        break;
      }

      r = ne_read_element(ctx, &id, &size);
      if (r != 1)
        break;

      if (element->flags & DESC_FLAG_OFFSET) {
        data_offset = (int64_t *) (ctx->ancestor->data + element->data_offset);
        *data_offset = ne_io_tell(ctx->io);
        if (*data_offset < 0) {
          r = -1;
          break;
        }
      }

      if (element->type == TYPE_MASTER) {
        if (element->flags & DESC_FLAG_MULTI) {
  			data_offset2 = ne_io_tell(ctx->io);
        	printf("master %lu %ld\n", size + 7, data_offset2 - 42);
          ne_read_master(ctx, element);
        } else {
          ne_read_single_master(ctx, element);
        }
        continue;
      } else {
        r = ne_read_simple(ctx, element, size);
        if (r < 0)
          break;
      }
    } else if (ne_is_ancestor_element(id, ctx->ancestor->previous)) {
      ctx->log(ctx, NESTEGG_LOG_DEBUG, "parent element %llx", id);
      if (top_level && ctx->ancestor->node == top_level) {
        ctx->log(ctx, NESTEGG_LOG_DEBUG, "*** parse about to back up past top_level");
        r = 1;
        break;
      }
      ne_ctx_pop(ctx);
    } else {
      r = ne_read_element(ctx, &id, &size);
      if (r != 1)
        break;

      if (id != ID_VOID && id != ID_CRC32)
        ctx->log(ctx, NESTEGG_LOG_DEBUG, "unknown element %llx", id);
      r = ne_io_read_skip(ctx->io, size);
      if (r != 1)
        break;
    }
  }

  if (r != 1)
    while (ctx->ancestor)
      ne_ctx_pop(ctx);

  return r;
}

static uint64_t
ne_xiph_lace_value(unsigned char ** np)
{
  uint64_t lace;
  uint64_t value;
  unsigned char * p = *np;

  lace = *p++;
  value = lace;
  while (lace == 255) {
    lace = *p++;
    value += lace;
  }

  *np = p;

  return value;
}

static int
ne_read_xiph_lace_value(nestegg_io * io, uint64_t * value, size_t * consumed)
{
  int r;
  uint64_t lace;

  r = ne_read_uint(io, &lace, 1);
  if (r != 1)
    return r;
  *consumed += 1;

  *value = lace;
  while (lace == 255) {
    r = ne_read_uint(io, &lace, 1);
    if (r != 1)
      return r;
    *consumed += 1;
    *value += lace;
  }

  return 1;
}

static int
ne_read_xiph_lacing(nestegg_io * io, size_t block, size_t * read, uint64_t n, uint64_t * sizes)
{
  int r;
  size_t i = 0;
  uint64_t sum = 0;

  while (--n) {
    r = ne_read_xiph_lace_value(io, &sizes[i], read);
    if (r != 1)
      return r;
    sum += sizes[i];
    i += 1;
  }

  if (*read + sum > block)
    return -1;

  /* Last frame is the remainder of the block. */
  sizes[i] = block - *read - sum;
  return 1;
}

static int
ne_read_ebml_lacing(nestegg_io * io, size_t block, size_t * read, uint64_t n, uint64_t * sizes)
{
  int r;
  uint64_t lace, sum, length;
  int64_t slace;
  size_t i = 0;

  r = ne_read_vint(io, &lace, &length);
  if (r != 1)
    return r;
  *read += length;

  sizes[i] = lace;
  sum = sizes[i];

  i += 1;
  n -= 1;

  while (--n) {
    r = ne_read_svint(io, &slace, &length);
    if (r != 1)
      return r;
    *read += length;
    sizes[i] = sizes[i - 1] + slace;
    sum += sizes[i];
    i += 1;
  }

  if (*read + sum > block)
    return -1;

  /* Last frame is the remainder of the block. */
  sizes[i] = block - *read - sum;
  return 1;
}

static uint64_t
ne_get_timecode_scale(nestegg * ctx)
{
  uint64_t scale;

  if (ne_get_uint(ctx->segment.info.timecode_scale, &scale) != 0)
    scale = 1000000;

  return scale;
}




static struct track_entry *
ne_find_track_entry(nestegg * ctx, unsigned int track)
{
  struct ebml_list_node * node;
  unsigned int tracks = 0;

  node = ctx->segment.tracks.track_entry.head;
  while (node) {
    assert(node->id == ID_TRACK_ENTRY);
    if (track == tracks)
      return node->data;
    tracks += 1;
    node = node->next;
  }

  return NULL;
}

static int
ne_read_block(nestegg * ctx, uint64_t block_id, uint64_t block_size, nestegg_packet ** data)
{
  int r;
  int64_t timecode, abs_timecode;
  nestegg_packet * pkt;
  struct cluster * cluster;
  struct frame * f, * last;
  struct track_entry * entry;
  double track_scale;
  uint64_t track, length, frame_sizes[256], cluster_tc, flags, frames, tc_scale, total;
  unsigned int i, lacing;
  size_t consumed = 0;

  *data = NULL;

  if (block_size > LIMIT_BLOCK)
    return -1;

  r = ne_read_vint(ctx->io, &track, &length);
  if (r != 1)
    return r;

//  if (track == 0 || track > ctx->track_count)
//    return -1;
  consumed += length;

  r = ne_read_int(ctx->io, &timecode, 2);
  if (r != 1)
    return r;
  consumed += 2;

  r = ne_read_uint(ctx->io, &flags, 1);
  if (r != 1)
    return r;
  consumed += 1;

  frames = 0;


  /* Flags are different between Block and SimpleBlock, but lacing is
     encoded the same way. */
  lacing = (flags & BLOCK_FLAGS_LACING) >> 1;

  switch (lacing) {
  case LACING_NONE:
    frames = 1;
    break;
  case LACING_XIPH:
  case LACING_FIXED:
  case LACING_EBML:
    r = ne_read_uint(ctx->io, &frames, 1);
    if (r != 1)
      return r;
    consumed += 1;
    frames += 1;
  }

  if (frames > 256)
    return -1;

  switch (lacing) {
  case LACING_NONE:
    frame_sizes[0] = block_size - consumed;
    break;
  case LACING_XIPH:
    if (frames == 1)
      return -1;
    r = ne_read_xiph_lacing(ctx->io, block_size, &consumed, frames, frame_sizes);
    if (r != 1)
      return r;
    break;
  case LACING_FIXED:
    if ((block_size - consumed) % frames)
      return -1;
    for (i = 0; i < frames; ++i)
      frame_sizes[i] = (block_size - consumed) / frames;
    break;
  case LACING_EBML:
    if (frames == 1)
      return -1;
    r = ne_read_ebml_lacing(ctx->io, block_size, &consumed, frames, frame_sizes);
    if (r != 1)
      return r;
    break;
  }

  /* Sanity check unlaced frame sizes against total block size. */
  total = consumed;
  for (i = 0; i < frames; ++i)
    total += frame_sizes[i];
  if (total > block_size)
    return -1;
  entry = ne_find_track_entry(ctx, 0);
  if (!entry)
    return -1;
  track_scale = 1.0;
  tc_scale = ne_get_timecode_scale(ctx);

  assert(ctx->segment.cluster.tail->id == ID_CLUSTER);
  cluster = ctx->segment.cluster.tail->data;
  if (ne_get_uint(cluster->timecode, &cluster_tc) != 0)
    return -1;
  abs_timecode = timecode + cluster_tc;
  if (abs_timecode < 0)
    return -1;

  pkt = ne_alloc(sizeof(*pkt));
  pkt->track = track - 1;
  pkt->timecode = abs_timecode * tc_scale * track_scale;
  ctx->log(ctx, NESTEGG_LOG_DEBUG, "%sblock t %lld pts %f f %llx frames: %llu",
           block_id == ID_BLOCK ? "" : "simple", pkt->track, pkt->timecode / 1e9, flags, frames);

  last = NULL;
  for (i = 0; i < frames; ++i) {
    if (frame_sizes[i] > LIMIT_FRAME) {
      nestegg_free_packet(pkt);
      return -1;
    }
    f = ne_alloc(sizeof(*f));
    f->data = ne_alloc(frame_sizes[i]);
    f->length = frame_sizes[i];
    r = ne_io_read(ctx->io, f->data, frame_sizes[i]);
    //printf("wantedt ot read %d got %d track is %d \n", frame_sizes[i], r , pkt->track);
    if (r != 1) {
      free(f->data);
      free(f);
      nestegg_free_packet(pkt);
      return -1;
    }

    if (!last)
      pkt->frame = f;
    else
      last->next = f;
    last = f;
  }

  *data = pkt;

  return 1;
}

static uint64_t
ne_buf_read_id(unsigned char const * p, size_t length)
{
  uint64_t id = 0;

  while (length--) {
    id <<= 8;
    id |= *p++;
  }

  return id;
}

static struct seek *
ne_find_seek_for_id(struct ebml_list_node * seek_head, uint64_t id)
{
  struct ebml_list * head;
  struct ebml_list_node * seek;
  struct ebml_binary binary_id;
  struct seek * s;

  while (seek_head) {
    assert(seek_head->id == ID_SEEK_HEAD);
    head = seek_head->data;
    seek = head->head;

    while (seek) {
      assert(seek->id == ID_SEEK);
      s = seek->data;

      if (ne_get_binary(s->id, &binary_id) == 0 &&
          ne_buf_read_id(binary_id.data, binary_id.length) == id)
        return s;

      seek = seek->next;
    }

    seek_head = seek_head->next;
  }

  return NULL;
}

static struct cue_point *
ne_find_cue_point_for_tstamp(struct ebml_list_node * cue_point, uint64_t scale, uint64_t tstamp)
{
  uint64_t time;
  struct cue_point * c, * prev = NULL;

  while (cue_point) {
    assert(cue_point->id == ID_CUE_POINT);
    c = cue_point->data;

    if (!prev)
      prev = c;

    if (ne_get_uint(c->time, &time) == 0 && time * scale > tstamp)
      break;

    prev = cue_point->data;
    cue_point = cue_point->next;
  }

  return prev;
}

static int
ne_is_suspend_element(uint64_t id)
{
  if (id == ID_SIMPLE_BLOCK || id == ID_BLOCK)
    return 1;
  return 0;
}

static void
ne_null_log_callback(nestegg * ctx, unsigned int severity, char const * fmt, ...)
{
  if (ctx && severity && fmt)
    return;
}


int
nestegg_write_track_data(nestegg * ctx, char *buffer)
{

	memcpy(buffer, ctx->tracks, ctx->tracks_length);
	return ctx->tracks_length;
}

// nestegg_init(nestegg ** context, nestegg_io io, nestegg_log callback, nestegg_info *info)

kradebml_t *kradebml_create() {

	kradebml_t *kradebml = calloc(1, sizeof(kradebml_t));

	kradebml->ebml = calloc(1, sizeof(EbmlGlobal));

	kradebml->next_tracknumber = 1;
	
	kradebml->total_tracks = 0;
	
  //int r;
  /*
  uint64_t id, version, docversion;
  struct ebml_list_node * track;
  char * doctype;
  nestegg * ctx;
	*/
  //if (!(io.read && io.seek && io.tell))
  //  return -1;

  kradebml->ctx = ne_alloc(sizeof(*kradebml->ctx));

  kradebml->ctx->io = ne_alloc(sizeof(*kradebml->ctx->io));
  *kradebml->ctx->io = kradebml->io;
  kradebml->ctx->log = log_callback;
  kradebml->ctx->alloc_pool = ne_pool_init();

  if (!kradebml->ctx->log)
    kradebml->ctx->log = ne_null_log_callback;
/*
  r = ne_peek_element(ctx, &id, NULL);
  if (r != 1) {
    nestegg_destroy(ctx);
    return -1;
  }

  if (id != ID_EBML) {
    nestegg_destroy(ctx);
    return -1;
  }

  ctx->log(ctx, NESTEGG_LOG_DEBUG, "ctx %p", ctx);

  ne_ctx_push(ctx, ne_top_level_elements, ctx);

  r = ne_parse(ctx, NULL);

  if (r != 1) {
    nestegg_destroy(ctx);
    return -1;
  }

  if (ne_get_uint(ctx->ebml.ebml_read_version, &version) != 0)
    version = 1;
  if (version != 1) {
    nestegg_destroy(ctx);
    return -1;
  }

  if (ne_get_string(ctx->ebml.doctype, &doctype) != 0)
    doctype = "matroska";
  if (strcmp(doctype, "webm") != 0) {
    nestegg_destroy(ctx);
    return -1;
  }

  if (ne_get_uint(ctx->ebml.doctype_read_version, &docversion) != 0)
    docversion = 1;
  if (docversion < 1 || docversion > 2) {
    nestegg_destroy(ctx);
    return -1;
  }

  if (!ctx->segment.tracks.track_entry.head) {
    nestegg_destroy(ctx);
    return -1;
  }

  track = ctx->segment.tracks.track_entry.head;
  ctx->track_count = 0;

  while (track) {
    ctx->track_count += 1;
    track = track->next;
  }
*/
//  *context = ctx;

	return kradebml;

}

void kradebml_destroy(kradebml_t *kradebml) {


	//printf("kradebml destroy\n");

	kradebml_close_output(kradebml);

	//if (kradebml->output_ring != NULL) {
	//	kradebml->shutdown = 1;
	//	pthread_cancel (kradebml->processing_thread);
	//	printf("kradebml pthread_cancel\n");
	//	//jack_ringbuffer_free (kradebml->output_ring);
	//}


  while (kradebml->ctx->ancestor)
    ne_ctx_pop(kradebml->ctx);
  ne_pool_destroy(kradebml->ctx->alloc_pool);
  free(kradebml->ctx->io);
  free(kradebml->ctx);



	//if (kradebml->input_ring != NULL) {
		//jack_ringbuffer_free (kradebml->input_ring);
	///}


	free(kradebml->ebml);
	free(kradebml);

	//printf("kradebml destroyed\n");

}

int
nestegg_duration(nestegg * ctx, uint64_t * duration)
{
  uint64_t tc_scale;
  double unscaled_duration;

  if (ne_get_float(ctx->segment.info.duration, &unscaled_duration) != 0)
    return -1;

  tc_scale = ne_get_timecode_scale(ctx);

  *duration = (uint64_t) (unscaled_duration * tc_scale);
  return 0;
}

int
nestegg_tstamp_scale(nestegg * ctx, uint64_t * scale)
{
  *scale = ne_get_timecode_scale(ctx);
  return 0;
}

int
nestegg_track_count(nestegg * ctx, unsigned int * tracks)
{
  *tracks = ctx->track_count;
  return 0;
}

int
nestegg_track_seek(nestegg * ctx, unsigned int track, uint64_t tstamp)
{
  int r;
  struct cue_point * cue_point;
  struct cue_track_positions * pos;
  struct saved_state state;
  struct seek * found;
  uint64_t seek_pos, tc_scale, t, id;
  struct ebml_list_node * node = ctx->segment.cues.cue_point.head;

  /* If there are no cues loaded, check for cues element in the seek head
     and load it. */
  if (!node) {
    found = ne_find_seek_for_id(ctx->segment.seek_head.head, ID_CUES);
    if (!found)
      return -1;

    if (ne_get_uint(found->position, &seek_pos) != 0)
      return -1;

    /* Save old parser state. */
    r = ne_ctx_save(ctx, &state);
    if (r != 0)
      return -1;

    /* Seek and set up parser state for segment-level element (Cues). */
    r = ne_io_seek(ctx->io, ctx->segment_offset + seek_pos, NESTEGG_SEEK_SET);
    if (r != 0)
      return -1;
    ctx->last_id = 0;
    ctx->last_size = 0;

    r = ne_read_element(ctx, &id, NULL);
    if (r != 1)
      return -1;

    if (id != ID_CUES)
      return -1;

    ctx->ancestor = NULL;
    ne_ctx_push(ctx, ne_top_level_elements, ctx);
    ne_ctx_push(ctx, ne_segment_elements, &ctx->segment);
    ne_ctx_push(ctx, ne_cues_elements, &ctx->segment.cues);
    /* Parser will run until end of cues element. */
    ctx->log(ctx, NESTEGG_LOG_DEBUG, "seek: parsing cue elements");
    r = ne_parse(ctx, ne_cues_elements);
    while (ctx->ancestor)
      ne_ctx_pop(ctx);

    /* Reset parser state to original state and seek back to old position. */
    if (ne_ctx_restore(ctx, &state) != 0)
      return -1;

    if (r < 0)
      return -1;
  }

  tc_scale = ne_get_timecode_scale(ctx);

  cue_point = ne_find_cue_point_for_tstamp(ctx->segment.cues.cue_point.head, tc_scale, tstamp);
  if (!cue_point)
    return -1;

  node = cue_point->cue_track_positions.head;

  seek_pos = 0;

  while (node) {
    assert(node->id == ID_CUE_TRACK_POSITIONS);
    pos = node->data;
    if (ne_get_uint(pos->track, &t) == 0 && t - 1 == track) {
      if (ne_get_uint(pos->cluster_position, &seek_pos) != 0)
        return -1;
      break;
    }
    node = node->next;
  }

  /* Seek and set up parser state for segment-level element (Cluster). */
  r = ne_io_seek(ctx->io, ctx->segment_offset + seek_pos, NESTEGG_SEEK_SET);
  if (r != 0)
    return -1;
  ctx->last_id = 0;
  ctx->last_size = 0;

  while (ctx->ancestor)
    ne_ctx_pop(ctx);

  ne_ctx_push(ctx, ne_top_level_elements, ctx);
  ne_ctx_push(ctx, ne_segment_elements, &ctx->segment);
  ctx->log(ctx, NESTEGG_LOG_DEBUG, "seek: parsing cluster elements");
  r = ne_parse(ctx, NULL);
  if (r != 1)
    return -1;

  if (!ne_is_suspend_element(ctx->last_id))
    return -1;

  return 0;
}

int
nestegg_track_type(nestegg * ctx, unsigned int track)
{
  struct track_entry * entry;
  uint64_t type;

  entry = ne_find_track_entry(ctx, track);
  if (!entry)
    return -1;

  if (ne_get_uint(entry->type, &type) != 0)
    return -1;

  if (type & 0x10)
    return NESTEGG_TRACK_SUBTITLE;

  if (type & TRACK_TYPE_VIDEO)
    return NESTEGG_TRACK_VIDEO;

  if (type & TRACK_TYPE_AUDIO)
    return NESTEGG_TRACK_AUDIO;

  return -1;
}

int
nestegg_track_codec_id(nestegg * ctx, unsigned int track)
{
  char * codec_id;
  struct track_entry * entry;

  entry = ne_find_track_entry(ctx, track);
  if (!entry)
    return -1;

  if (ne_get_string(entry->codec_id, &codec_id) != 0)
    return -1;

  if (strcmp(codec_id, TRACK_ID_VP8) == 0)
    return NESTEGG_CODEC_VP8;

  if (strcmp(codec_id, TRACK_ID_VORBIS) == 0)
    return NESTEGG_CODEC_VORBIS;

  if (strcmp(codec_id, TRACK_ID_OPUS) == 0)
    return NESTEGG_CODEC_OPUS;
    
  if (strcmp(codec_id, TRACK_ID_FLAC) == 0)
    return NESTEGG_CODEC_FLAC;

  if (strcmp(codec_id, TRACK_ID_DIRAC) == 0)
    return NESTEGG_CODEC_DIRAC;    

  if (strcmp(codec_id, TRACK_ID_THEORA) == 0)
    return NESTEGG_CODEC_THEORA;

  return -1;
}

char *krad_ebml_track_codec_string(kradebml_t *kradebml, unsigned int track) {

	char * codec_id;
	struct track_entry * entry;

	entry = ne_find_track_entry(kradebml->ctx, track);
	if (!entry)
	return "Err";

	if (ne_get_string(entry->codec_id, &codec_id) != 0)
	return "Unset";

	if (strcmp(codec_id, TRACK_ID_VP8) == 0)
	return "VP8";

	if (strcmp(codec_id, TRACK_ID_VORBIS) == 0)
	return "Vorbis";

	if (strcmp(codec_id, TRACK_ID_OPUS) == 0)
	return "Opus";

	if (strcmp(codec_id, TRACK_ID_FLAC) == 0)
	return "FLAC";

	if (strcmp(codec_id, TRACK_ID_DIRAC) == 0)
	return "Dirac";    

	if (strcmp(codec_id, TRACK_ID_THEORA) == 0)
	return "Theora";

	return codec_id + 2;

}

char *krad_ebml_track_codec_string_orig(kradebml_t *kradebml, unsigned int track) {

	char * codec_id;
	struct track_entry * entry;

	entry = ne_find_track_entry(kradebml->ctx, track);
	if (!entry)
	return "Err";

	if (ne_get_string(entry->codec_id, &codec_id) != 0)
	return "Unset";

	return codec_id;

}

int krad_ebml_track_codec(kradebml_t *kradebml, unsigned int track) {

	char * codec_id;
	struct track_entry * entry;

	entry = ne_find_track_entry(kradebml->ctx, track);
	if (!entry)
	return -1;

	if (ne_get_string(entry->codec_id, &codec_id) != 0)
	return -1;

	if (strcmp(codec_id, TRACK_ID_VP8) == 0)
	return KRAD_VP8;

	if (strcmp(codec_id, TRACK_ID_VORBIS) == 0)
	return KRAD_VORBIS;

	if (strcmp(codec_id, TRACK_ID_OPUS) == 0)
	return KRAD_OPUS;

	if (strcmp(codec_id, TRACK_ID_FLAC) == 0)
	return KRAD_FLAC;

	if (strcmp(codec_id, TRACK_ID_DIRAC) == 0)
	return KRAD_DIRAC;    

	if (strcmp(codec_id, TRACK_ID_THEORA) == 0)
	return KRAD_THEORA;    

	return -1;

}

int
nestegg_track_codec_data_count(nestegg * ctx, unsigned int track,
                               unsigned int * count)
{
  struct track_entry * entry;
  struct ebml_binary codec_private;
  unsigned char * p;

  *count = 0;

  entry = ne_find_track_entry(ctx, track);
  if (!entry)
    return -1;


	if ((nestegg_track_codec_id(ctx, track) != NESTEGG_CODEC_VORBIS) && ((nestegg_track_codec_id(ctx, track) != NESTEGG_CODEC_OPUS)) && ((nestegg_track_codec_id(ctx, track) != NESTEGG_CODEC_FLAC)) && ((nestegg_track_codec_id(ctx, track) != NESTEGG_CODEC_THEORA))) {
	  return -1;
	}

	if ((nestegg_track_codec_id(ctx, track) == NESTEGG_CODEC_FLAC)) {
		
		*count = 1;
		return 0;
	}
	
  if (ne_get_binary(entry->codec_private, &codec_private) != 0)
    return -1;

  if (codec_private.length < 1)
    return -1;

	printf("codec_private.length is %zu\n", codec_private.length);

  p = codec_private.data;
  *count = *p + 1;

  if (*count > 3)
    return -1;

  return 0;
}

int
nestegg_track_codec_data(nestegg * ctx, unsigned int track, unsigned int item,
                         unsigned char ** data, size_t * length)
{
  struct track_entry * entry;
  struct ebml_binary codec_private;
  uint64_t sizes[3], total;
  unsigned char * p;
  unsigned int count, i;

  *data = NULL;
  *length = 0;

  entry = ne_find_track_entry(ctx, track);
  if (!entry)
    return -1;

	//if ((nestegg_track_codec_id(ctx, track) != NESTEGG_CODEC_VORBIS) && ((nestegg_track_codec_id(ctx, track) != NESTEGG_CODEC_OPUS)) && ((nestegg_track_codec_id(ctx, track) != NESTEGG_CODEC_FLAC))) {
	//  return -1;
	//}



	if ((nestegg_track_codec_id(ctx, track) == NESTEGG_CODEC_OPUS) || (nestegg_track_codec_id(ctx, track) == NESTEGG_CODEC_FLAC)) {
	   if (ne_get_binary(entry->codec_private, &codec_private) != 0)
    		return -1;
	
  if (codec_private.length < 1) {
    return -1;
	}
	printf("zzlength is %zu\n", codec_private.length);
		*length = codec_private.length;
		*data = codec_private.data;
	

	return 0;
	}

  if (ne_get_binary(entry->codec_private, &codec_private) != 0)
    return -1;

  p = codec_private.data;
  count = *p++ + 1;

  if (count > 3)
    return -1;

  i = 0;
  total = 0;
  while (--count) {
    sizes[i] = ne_xiph_lace_value(&p);
    total += sizes[i];
    i += 1;
  }
  sizes[i] = codec_private.length - total - (p - codec_private.data);

  for (i = 0; i < item; ++i) {
    if (sizes[i] > LIMIT_FRAME)
      return -1;
    p += sizes[i];
  }
  *data = p;
  *length = sizes[item];

  return 0;
}

int
nestegg_track_video_params(nestegg * ctx, unsigned int track,
                           nestegg_video_params * params)
{
  struct track_entry * entry;
  uint64_t value;

  memset(params, 0, sizeof(*params));

  entry = ne_find_track_entry(ctx, track);
  if (!entry)
    return -1;

  if (nestegg_track_type(ctx, track) != NESTEGG_TRACK_VIDEO)
    return -1;

  value = 0;
  ne_get_uint(entry->video.stereo_mode, &value);
  if (value <= NESTEGG_VIDEO_STEREO_TOP_BOTTOM ||
      value == NESTEGG_VIDEO_STEREO_RIGHT_LEFT)
    params->stereo_mode = value;

  if (ne_get_uint(entry->video.pixel_width, &value) != 0)
    return -1;
  params->width = value;

  if (ne_get_uint(entry->video.pixel_height, &value) != 0)
    return -1;
  params->height = value;

  value = 0;
  ne_get_uint(entry->video.pixel_crop_bottom, &value);
  params->crop_bottom = value;

  value = 0;
  ne_get_uint(entry->video.pixel_crop_top, &value);
  params->crop_top = value;

  value = 0;
  ne_get_uint(entry->video.pixel_crop_left, &value);
  params->crop_left = value;

  value = 0;
  ne_get_uint(entry->video.pixel_crop_right, &value);
  params->crop_right = value;

  value = params->width;
  ne_get_uint(entry->video.display_width, &value);
  params->display_width = value;

  value = params->height;
  ne_get_uint(entry->video.display_height, &value);
  params->display_height = value;

  return 0;
}

int
nestegg_track_audio_params(nestegg * ctx, unsigned int track,
                           nestegg_audio_params * params)
{
  struct track_entry * entry;
  uint64_t value;

  memset(params, 0, sizeof(*params));

  entry = ne_find_track_entry(ctx, track);
  if (!entry)
    return -1;

  if (nestegg_track_type(ctx, track) != NESTEGG_TRACK_AUDIO)
    return -1;

  params->rate = 8000;
  ne_get_float(entry->audio.sampling_frequency, &params->rate);

  value = 1;
  ne_get_uint(entry->audio.channels, &value);
  params->channels = value;

  value = 16;
  ne_get_uint(entry->audio.bit_depth, &value);
  params->depth = value;

  return 0;
}

int
nestegg_read_packet(nestegg * ctx, nestegg_packet ** pkt)
{
  int r;
  uint64_t id, size;

  *pkt = NULL;

  for (;;) {
    r = ne_peek_element(ctx, &id, &size);
    if (r != 1)
      return r;

    /* Any DESC_FLAG_SUSPEND fields must be handled here. */
    if (ne_is_suspend_element(id)) {
      r = ne_read_element(ctx, &id, &size);
      if (r != 1)
        return r;

      /* The only DESC_FLAG_SUSPEND fields are Blocks and SimpleBlocks, which we
         handle directly. */
      r = ne_read_block(ctx, id, size, pkt);
      return r;
    }

    r =  ne_parse(ctx, NULL);
    if (r != 1)
      return r;
  }

  return 1;
}

void
nestegg_free_packet(nestegg_packet * pkt)
{
  struct frame * frame;

  while (pkt->frame) {
    frame = pkt->frame;
    pkt->frame = frame->next;
    free(frame->data);
    free(frame);
  }

 free(pkt);
}

int
nestegg_packet_track(nestegg_packet * pkt, unsigned int * track)
{
  *track = pkt->track;
  return 0;
}

int
nestegg_packet_tstamp(nestegg_packet * pkt, uint64_t * tstamp)
{
  *tstamp = pkt->timecode;
  return 0;
}

int
nestegg_packet_count(nestegg_packet * pkt, unsigned int * count)
{
  struct frame * f = pkt->frame;

  *count = 0;

  while (f) {
    *count += 1;
    f = f->next;
  }

  return 0;
}

int
nestegg_packet_data(nestegg_packet * pkt, unsigned int item,
                    unsigned char ** data, size_t * length)
{
  struct frame * f = pkt->frame;
  unsigned int count = 0;

  *data = NULL;
  *length = 0;

  while (f) {
    if (count == item) {
      *data = f->data;
      *length = f->length;
      return 0;
    }
    count += 1;
    f = f->next;
  }

  return -1;
}

int kradebml_read_video(kradebml_t *kradebml, unsigned char *buffer) {


	int i;
	int size;

 	if (nestegg_read_packet(kradebml->ctx, &kradebml->pkt) > 0) {
    nestegg_packet_track(kradebml->pkt, &kradebml->pkt_track);
    nestegg_packet_count(kradebml->pkt, &kradebml->pkt_cnt);
    nestegg_packet_tstamp(kradebml->pkt, &kradebml->pkt_tstamp);


	//fprintf(stderr, "t %u pts %f frames %u: ", kradebml->pkt_track, kradebml->pkt_tstamp / 1e9, kradebml->pkt_cnt);


	if (kradebml->pkt_track == kradebml->video_track) {

		for (i = 0; i < kradebml->pkt_cnt; ++i) {
		  nestegg_packet_data(kradebml->pkt, i, &kradebml->ptr, &kradebml->size);

		  //fprintf(stderr, "%u ", (unsigned int) size);

		}

		//fprintf(stderr, "\n");
		
		size = kradebml->size;
		memcpy(buffer, kradebml->ptr, size);


		nestegg_free_packet(kradebml->pkt);
		
		
			return size;
			
	} else {
	
		return kradebml_read_video(kradebml, buffer);
	
	}
    
  } else {
  
  	printf("bumkis\n");
  
  }

	return 0;

}

int kradebml_read_audio(kradebml_t *kradebml, unsigned char *buffer) {


	int i;
	int size;

 	if (nestegg_read_packet(kradebml->ctx, &kradebml->pkt) > 0) {
    nestegg_packet_track(kradebml->pkt, &kradebml->pkt_track);
    nestegg_packet_count(kradebml->pkt, &kradebml->pkt_cnt);
    nestegg_packet_tstamp(kradebml->pkt, &kradebml->pkt_tstamp);


	fprintf(stderr, "t %u pts %f frames %u: ", kradebml->pkt_track, kradebml->pkt_tstamp / 1e9, kradebml->pkt_cnt);


    for (i = 0; i < kradebml->pkt_cnt; ++i) {
      nestegg_packet_data(kradebml->pkt, i, &kradebml->ptr, &kradebml->size);

      //fprintf(stderr, "%u ", (unsigned int) size);

    }

    //fprintf(stderr, "\n");
    
    size = kradebml->size;
    memcpy(buffer, kradebml->ptr, size);


    nestegg_free_packet(kradebml->pkt);
    
    
    	return size;
    
  } else {
  
  	printf("bumkis\n");
  
  }

	return 0;

}

int kradebml_read_audio_header(kradebml_t *kradebml, int tracknum, unsigned char *buffer) {

	printf("read audio header\n");
	nestegg_track_codec_data(kradebml->ctx, tracknum, 0, &kradebml->codec_data, &kradebml->length);
	printf("read audio header end \n");
	memcpy(buffer, kradebml->codec_data, kradebml->length);

	return kradebml->length;
}


void kradebml_debug(kradebml_t *kradebml) {

	int i, j;

  nestegg_track_count(kradebml->ctx, &kradebml->tracks);
  nestegg_duration(kradebml->ctx, &kradebml->duration);

  fprintf(stderr, "media has %u tracks and duration %fs\n", kradebml->tracks, kradebml->duration / 1e9);

  for (i = 0; i < kradebml->tracks; ++i) {
    kradebml->type = nestegg_track_type(kradebml->ctx, i);

    fprintf(stderr, "track %u: type: %d codec: %d", i,
            kradebml->type, nestegg_track_codec_id(kradebml->ctx, i));

    nestegg_track_codec_data_count(kradebml->ctx, i, &kradebml->data_items);
    
    printf("blah data items %d\n", kradebml->data_items );
    
    for (j = 0; j < kradebml->data_items; ++j) {
      nestegg_track_codec_data(kradebml->ctx, i, j, &kradebml->codec_data, &kradebml->length);

	fprintf(stderr, "%d (%p, %u)", j, kradebml->codec_data, (unsigned int) kradebml->length);

    }
    if (kradebml->type == NESTEGG_TRACK_VIDEO) {
      nestegg_track_video_params(kradebml->ctx, i, &kradebml->vparams);

	kradebml->video_track = i;

      fprintf(stderr, " video: %ux%u (d: %ux%u %ux%ux%ux%u)",
              kradebml->vparams.width, kradebml->vparams.height,
              kradebml->vparams.display_width, kradebml->vparams.display_height,
              kradebml->vparams.crop_top, kradebml->vparams.crop_left, kradebml->vparams.crop_bottom, kradebml->vparams.crop_right);



	// the kl


		nestegg_track_codec_data(kradebml->ctx, i, 0, &kradebml->codec_data, &kradebml->length);
      	fprintf(stderr, "%d (%s, %zu)", j, kradebml->codec_data, kradebml->length);
      	
      	kradebml->theora_header1_len = kradebml->length;
      	memcpy(kradebml->theora_header1, kradebml->codec_data, kradebml->theora_header1_len);
  
  
		nestegg_track_codec_data(kradebml->ctx, i, 1, &kradebml->codec_data, &kradebml->length);
      	fprintf(stderr, "%d (%s, %zu)", j, kradebml->codec_data, kradebml->length);
      	
      	kradebml->theora_header2_len = kradebml->length;
      	memcpy(kradebml->theora_header2, kradebml->codec_data, kradebml->theora_header2_len);
      	      	
		nestegg_track_codec_data(kradebml->ctx, i, 2, &kradebml->codec_data, &kradebml->length);
      	fprintf(stderr, "%d (%s, %zu)", j, kradebml->codec_data, kradebml->length);

      	kradebml->theora_header3_len = kradebml->length;      	
      	memcpy(kradebml->theora_header3, kradebml->codec_data, kradebml->theora_header3_len);




	// th kl


    } else if (kradebml->type == NESTEGG_TRACK_AUDIO) {
      nestegg_track_audio_params(kradebml->ctx, i, &kradebml->aparams);


	kradebml->audio_track = i;


      fprintf(stderr, " audio: %.2fhz %u bit %u channels",
              kradebml->aparams.rate, kradebml->aparams.depth, kradebml->aparams.channels);


		nestegg_track_codec_data(kradebml->ctx, i, 0, &kradebml->codec_data, &kradebml->length);
      	fprintf(stderr, "%d (%s, %zu)", j, kradebml->codec_data, kradebml->length);
      	
      	kradebml->vorbis_header1_len = kradebml->length;
      	memcpy(kradebml->vorbis_header1, kradebml->codec_data, kradebml->vorbis_header1_len);
  
  
		nestegg_track_codec_data(kradebml->ctx, i, 1, &kradebml->codec_data, &kradebml->length);
      	fprintf(stderr, "%d (%s, %zu)", j, kradebml->codec_data, kradebml->length);
      	
      	kradebml->vorbis_header2_len = kradebml->length;
      	memcpy(kradebml->vorbis_header2, kradebml->codec_data, kradebml->vorbis_header2_len);
      	      	
		nestegg_track_codec_data(kradebml->ctx, i, 2, &kradebml->codec_data, &kradebml->length);
      	fprintf(stderr, "%d (%s, %zu)", j, kradebml->codec_data, kradebml->length);

      	kradebml->vorbis_header3_len = kradebml->length;      	
      	memcpy(kradebml->vorbis_header3, kradebml->codec_data, kradebml->vorbis_header3_len);

      	

    }

    fprintf(stderr, "\n");


	}

}

char *kradebml_input_info(kradebml_t *kradebml) {

	char *i;
	int *p;
	
	int t;
	
	char *codec;
	int codec_type;
	char *doctype;
	
	kradebml->input_info_pos = 0;

	i = kradebml->input_info;
	p = &kradebml->input_info_pos;

	*p += sprintf(i + *p, "Krad EBML File Info\n\n");
	
	*p += sprintf(i + *p, "Filename: \t%s\n", kradebml->filename);

	nestegg_track_count(kradebml->ctx, &kradebml->tracks);
	nestegg_duration(kradebml->ctx, &kradebml->duration);

	ne_get_string(kradebml->ctx->ebml.doctype, &doctype);

	*p += sprintf(i + *p, "Type:   \t%s\n", doctype);
	*p += sprintf(i + *p, "Duration: \t%.3fs\n", kradebml->duration / 1e9);
	*p += sprintf(i + *p, "Tracks: \t%u\n", kradebml->tracks);

	for (t = 0; t < kradebml->tracks; t++) {

		codec_type = nestegg_track_type(kradebml->ctx, t);

		codec = krad_ebml_track_codec_string(kradebml, t);		

		switch (codec_type) {
		
			case NESTEGG_TRACK_VIDEO:			
				nestegg_track_video_params(kradebml->ctx, t, &kradebml->vparams);
				*p += sprintf(i + *p, "Track %d Video: \t%d x %d %s\n", t + 1, kradebml->vparams.width, kradebml->vparams.height, codec);
    			break;
    			
	    	case NESTEGG_TRACK_AUDIO:	
				nestegg_track_audio_params(kradebml->ctx, t, &kradebml->aparams);
				*p += sprintf(i + *p, "Track %d Audio: \t%.0fhz %u bit %u channels %s\n", t + 1, kradebml->aparams.rate, kradebml->aparams.depth, kradebml->aparams.channels, codec);
				break;

	    	case NESTEGG_TRACK_SUBTITLE:	
				*p += sprintf(i + *p, "Track %d Subs: \t%s\n", t + 1, codec);
				break;

			default:
				*p += sprintf(i + *p, "Track %d Unknown: \t%s\n", t + 1, codec);
				break;
		}
	}

	*p += sprintf(i + *p, "\n");
	
	return kradebml->input_info;

}


char *kradebml_clone_tracks(kradebml_t *kradebml, kradebml_t *kradebml_output) {

	char *i;
	int *p;
	
	int t;
	
	char *temp;
	int templen;
	int dem;
	
	char *codec;
	int codec_type;
	char *doctype;
	
	kradebml->input_info_pos = 0;

	i = kradebml->input_info;
	p = &kradebml->input_info_pos;

	*p += sprintf(i + *p, "Krad EBML File Info\n\n");
	
	*p += sprintf(i + *p, "Filename: \t%s\n", kradebml->filename);

	nestegg_track_count(kradebml->ctx, &kradebml->tracks);
	nestegg_duration(kradebml->ctx, &kradebml->duration);

	ne_get_string(kradebml->ctx->ebml.doctype, &doctype);

	*p += sprintf(i + *p, "Type:   \t%s\n", doctype);
	*p += sprintf(i + *p, "Duration: \t%.3fs\n", kradebml->duration / 1e9);
	*p += sprintf(i + *p, "Tracks: \t%u\n", kradebml->tracks);

	for (t = 0; t < kradebml->tracks; t++) {

		codec_type = nestegg_track_type(kradebml->ctx, t);

		codec = krad_ebml_track_codec_string_orig(kradebml, t);		

		switch (codec_type) {
		
			case NESTEGG_TRACK_VIDEO:			
				nestegg_track_video_params(kradebml->ctx, t, &kradebml->vparams);
				*p += sprintf(i + *p, "Track %d Video: \t%d x %d %s\n", t + 1, kradebml->vparams.width, kradebml->vparams.height, codec);
				//nestegg_track_codec_data(kradebml->ctx, t, 0, &kradebml->codec_data, &kradebml->length);

				nestegg_track_codec_data(kradebml->ctx, t, 0, &kradebml->codec_data, &kradebml->length);
				kradebml_add_video_track_with_priv(kradebml_output, codec, 30, kradebml->vparams.width, kradebml->vparams.height, kradebml->codec_data, kradebml->length);
    			break;
    			
	    	case NESTEGG_TRACK_AUDIO:	
				nestegg_track_audio_params(kradebml->ctx, t, &kradebml->aparams);
				*p += sprintf(i + *p, "Track %d Audio: \t%.0fhz %u bit %u channels %s\n", t + 1, kradebml->aparams.rate, kradebml->aparams.depth, kradebml->aparams.channels, codec);
				if (strncmp("A_VORBIS", codec, 8) == 0) {

					templen = 3;
					temp = malloc(templen + kradebml->vorbis_header1_len + kradebml->vorbis_header2_len + kradebml->vorbis_header3_len);
	

					temp[0] = 0x02;
					dem = kradebml->vorbis_header1_len;
					temp[1] = (char)dem;
					dem = kradebml->vorbis_header2_len;
					temp[2] = (char)dem;
					memcpy(temp + templen, kradebml->vorbis_header1, kradebml->vorbis_header1_len);
					memcpy(temp + templen + kradebml->vorbis_header1_len, kradebml->vorbis_header2, kradebml->vorbis_header2_len);
					memcpy(temp + templen + kradebml->vorbis_header1_len + kradebml->vorbis_header2_len, kradebml->vorbis_header3, kradebml->vorbis_header3_len);

					nestegg_track_codec_data(kradebml->ctx, t, 0, &kradebml->codec_data, &kradebml->length);
					kradebml_add_audio_track(kradebml_output, codec, kradebml->aparams.rate, kradebml->aparams.channels, temp, templen + kradebml->vorbis_header1_len + kradebml->vorbis_header2_len + kradebml->vorbis_header3_len);
					free(temp);
				} else {
					nestegg_track_codec_data(kradebml->ctx, t, 0, &kradebml->codec_data, &kradebml->length);
					kradebml_add_audio_track(kradebml_output, codec, kradebml->aparams.rate, kradebml->aparams.channels, kradebml->codec_data, kradebml->length);
				}
				break;

	    	case NESTEGG_TRACK_SUBTITLE:	
				*p += sprintf(i + *p, "Track %d Subs: \t%s\n", t + 1, codec);
				kradebml_add_subtitle_track(kradebml_output, codec);
				break;

			default:
				*p += sprintf(i + *p, "Track %d Unknown: \t%s\n", t + 1, codec);
				break;
		}
	}

	*p += sprintf(i + *p, "\n");
	
	return kradebml->input_info;

}


int kradebml_open_input_advanced(kradebml_t *kradebml) {

	int r;

	uint64_t id, version, docversion;
	struct ebml_list_node *track;
	char *doctype;
	/*
	kradebml->io.userdata = kradebml;

	kradebml->io.read = kradebml_stdio_read;
	kradebml->io.seek = kradebml_stdio_seek;
	kradebml->io.tell = kradebml_stdio_tell;
	kradebml->io.write = kradebml_stdio_write;

	strncpy(kradebml->filename, filename, 512);
	
	kradebml_stdio_open(kradebml, "r");
	
	*/
	
	sprintf(kradebml->input_name, "buffer");
	kradebml->input_type = KRADEBML_BUFFER;
	
  *kradebml->ctx->io = kradebml->io;

	if (kradebml->info.cluster_at != NULL) {
		printf("Got Info Callbacks\n");
		kradebml->ctx->info = &kradebml->info;
	}

	//if (!kradebml->ctx->log)
	//	kradebml->ctx->log = ne_null_log_callback;

	r = ne_peek_element(kradebml->ctx, &id, NULL);

	if (r != 1) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (id != ID_EBML) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	kradebml->ctx->log(kradebml->ctx, NESTEGG_LOG_DEBUG, "ctx %p", kradebml->ctx);

	ne_ctx_push(kradebml->ctx, ne_top_level_elements, kradebml->ctx);

	r = ne_parse(kradebml->ctx, NULL);

	if (r != 1) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (ne_get_uint(kradebml->ctx->ebml.ebml_read_version, &version) != 0)
		version = 1;
		if (version != 1) {
			//nestegg_destroy(kradebml->ctx);
			return -1;
	}

	//if (ne_get_string(ctx->ebml.doctype, &doctype) != 0)
	//	doctype = "matroska";
	//if (strcmp(doctype, "webm") != 0) {
	//	nestegg_destroy(ctx);
	//	return -1;
	//}

	if (ne_get_uint(kradebml->ctx->ebml.doctype_read_version, &docversion) != 0)
	docversion = 1;
	if (docversion < 1 || docversion > 2) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (!kradebml->ctx->segment.tracks.track_entry.head) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	track = kradebml->ctx->segment.tracks.track_entry.head;
	kradebml->ctx->track_count = 0;

	while (track) {
		kradebml->ctx->track_count += 1;
		track = track->next;
	}


	return 0;
}

int kradebml_open_input_file(kradebml_t *kradebml, char *filename) {

	int r;

	uint64_t id, version, docversion;
	struct ebml_list_node *track;
	char *doctype;
	
	kradebml->io.userdata = kradebml;

	kradebml->io.read = kradebml_stdio_read;
	kradebml->io.seek = kradebml_stdio_seek;
	kradebml->io.tell = kradebml_stdio_tell;
	kradebml->io.write = kradebml_stdio_write;

	strncpy(kradebml->filename, filename, 512);
	
	strncpy(kradebml->input_name, filename, 512);
	kradebml->input_type = KRADEBML_FILE;
	
	kradebml_stdio_open(kradebml, "r");
	*kradebml->ctx->io = kradebml->io;

	kradebml->info.cluster_at = cluster_at;
	kradebml->info.userdata = kradebml;
	
	if (kradebml->info.cluster_at != NULL) {
		kradebml->ctx->info = &kradebml->info;
	}

	//if (!kradebml->ctx->log)
	//	kradebml->ctx->log = ne_null_log_callback;

	r = ne_peek_element(kradebml->ctx, &id, NULL);

	if (r != 1) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (id != ID_EBML) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	kradebml->ctx->log(kradebml->ctx, NESTEGG_LOG_DEBUG, "ctx %p", kradebml->ctx);

	ne_ctx_push(kradebml->ctx, ne_top_level_elements, kradebml->ctx);

	r = ne_parse(kradebml->ctx, NULL);

	if (r != 1) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (ne_get_uint(kradebml->ctx->ebml.ebml_read_version, &version) != 0)
		version = 1;
		if (version != 1) {
			//nestegg_destroy(kradebml->ctx);
			return -1;
	}

	//if (ne_get_string(ctx->ebml.doctype, &doctype) != 0)
	//	doctype = "matroska";
	//if (strcmp(doctype, "webm") != 0) {
	//	nestegg_destroy(ctx);
	//	return -1;
	//}

	if (ne_get_uint(kradebml->ctx->ebml.doctype_read_version, &docversion) != 0)
	docversion = 1;
	if (docversion < 1 || docversion > 2) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (!kradebml->ctx->segment.tracks.track_entry.head) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	track = kradebml->ctx->segment.tracks.track_entry.head;
	kradebml->ctx->track_count = 0;

	while (track) {
		kradebml->ctx->track_count += 1;
		track = track->next;
	}


	return 0;
}



int kradebml_open_input_stream(kradebml_t *kradebml, char *host, int port, char *mount) {


	int r;

	uint64_t id, version, docversion;
	struct ebml_list_node *track;
	char *doctype;

	kradebml->io.userdata = kradebml;

	kradebml->io.read = kradebml_stream_read;
	kradebml->io.seek = kradebml_stream_seek;
	kradebml->io.tell = kradebml_stream_tell;
	kradebml->io.write = kradebml_stream_write;

	strncpy(kradebml->filename, mount, 512);
	kradebml->server = server_create(host, port, mount, "monkey");
	kradebml_stream_open(kradebml);
	
	sprintf(kradebml->input_name, "%s:%d%s", host, port, mount);
	kradebml->input_type = KRADEBML_STREAM;
	
	char getstring[512];
	
	sprintf(getstring, "GET %s HTTP/1.0\r\n\r\n", mount);
	
	server_send(kradebml->server, getstring, strlen(getstring));
	
	int end_http_headers = 0;
	char buf[8];
	while (end_http_headers != 4) {
	
		kradebml_stream_read(buf, 1, kradebml);
	
		printf("%c", buf[0]);
	
		if ((buf[0] == '\n') || (buf[0] == '\r')) {
			end_http_headers++;
		} else {
			end_http_headers = 0;
		}
	
	}
	
	
	usleep(150000);
	
  *kradebml->ctx->io = kradebml->io;

	//if (!kradebml->ctx->log)
	//	kradebml->ctx->log = ne_null_log_callback;

	r = ne_peek_element(kradebml->ctx, &id, NULL);

	if (r != 1) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (id != ID_EBML) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	kradebml->ctx->log(kradebml->ctx, NESTEGG_LOG_DEBUG, "ctx %p", kradebml->ctx);

	ne_ctx_push(kradebml->ctx, ne_top_level_elements, kradebml->ctx);

	r = ne_parse(kradebml->ctx, NULL);

	if (r != 1) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (ne_get_uint(kradebml->ctx->ebml.ebml_read_version, &version) != 0)
		version = 1;
		if (version != 1) {
			//nestegg_destroy(kradebml->ctx);
			return -1;
	}

	//if (ne_get_string(ctx->ebml.doctype, &doctype) != 0)
	//	doctype = "matroska";
	//if (strcmp(doctype, "webm") != 0) {
	//	nestegg_destroy(ctx);
	//	return -1;
	//}

	if (ne_get_uint(kradebml->ctx->ebml.doctype_read_version, &docversion) != 0)
	docversion = 1;
	if (docversion < 1 || docversion > 2) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (!kradebml->ctx->segment.tracks.track_entry.head) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	track = kradebml->ctx->segment.tracks.track_entry.head;
	kradebml->ctx->track_count = 0;

	while (track) {
		kradebml->ctx->track_count += 1;
		track = track->next;
	}
	
	
	return 0;
}

void *processing_thread(void *arg) {

	kradebml_t *kradebml = (kradebml_t *)arg;

	printf("kradebml processing thread begins\n");

	int i, j, len;
	
	char buffer[8192 * 4];
	
	int r;

	uint64_t id, version, docversion;
	struct ebml_list_node *track;
	char *doctype;
	
	usleep(650000);
	
  *kradebml->ctx->io = kradebml->io;

	//if (!kradebml->ctx->log)
	//	kradebml->ctx->log = ne_null_log_callback;

	r = ne_peek_element(kradebml->ctx, &id, NULL);

	if (r != 1) {
		//nestegg_destroy(kradebml->ctx);
		return -1;
	}

	if (id != ID_EBML) {
		//nestegg_destroy(kradebml->ctx);
		return NULL;
	}

	kradebml->ctx->log(kradebml->ctx, NESTEGG_LOG_DEBUG, "ctx %p", kradebml->ctx);

	ne_ctx_push(kradebml->ctx, ne_top_level_elements, kradebml->ctx);

	r = ne_parse(kradebml->ctx, NULL);

	if (r != 1) {
		//nestegg_destroy(kradebml->ctx);
		return NULL;
	}

	if (ne_get_uint(kradebml->ctx->ebml.ebml_read_version, &version) != 0)
		version = 1;
		if (version != 1) {
			//nestegg_destroy(kradebml->ctx);
			return NULL;
	}

	//if (ne_get_string(ctx->ebml.doctype, &doctype) != 0)
	//	doctype = "matroska";
	//if (strcmp(doctype, "webm") != 0) {
	//	nestegg_destroy(ctx);
	//	return -1;
	//}

	if (ne_get_uint(kradebml->ctx->ebml.doctype_read_version, &docversion) != 0)
	docversion = 1;
	if (docversion < 1 || docversion > 2) {
		//nestegg_destroy(kradebml->ctx);
		return NULL;
	}

	if (!kradebml->ctx->segment.tracks.track_entry.head) {
		//nestegg_destroy(kradebml->ctx);
		return NULL;
	}

	track = kradebml->ctx->segment.tracks.track_entry.head;
	kradebml->ctx->track_count = 0;

	while (track) {
		kradebml->ctx->track_count += 1;
		track = track->next;
	}
	

	nestegg_track_count(kradebml->ctx, &kradebml->tracks);
	nestegg_duration(kradebml->ctx, &kradebml->duration);

	printf("media has %u tracks and duration %fs\n", kradebml->tracks, kradebml->duration / 1e9);

	
	while (((len = kradebml_read_audio(kradebml, buffer)) > 0) && (kradebml->shutdown == 0)) {
	
		usleep(5000);
	
		if (kradebml->shutdown == 1) {
		
			break;
		}
	
	}

	printf("kradebml processing thread ends\n");

	return NULL;

}

kradebml_t *kradebml_create_feedbuffer() {

	kradebml_t *kradebml = calloc(1, sizeof(kradebml_t));

	kradebml->ebml = calloc(1, sizeof(EbmlGlobal));


  //int r;
  /*
  uint64_t id, version, docversion;
  struct ebml_list_node * track;
  char * doctype;
  nestegg * ctx;
	*/
  //if (!(io.read && io.seek && io.tell))
  //  return -1;


//	kradebml->input_ring = jack_ringbuffer_create(5000000);
//	kradebml->output_ring = jack_ringbuffer_create(5000000);
  
  		kradebml->io.read = kradebml_feedbuffer_read;
		kradebml->io.seek = kradebml_feedbuffer_seek;
		kradebml->io.tell = kradebml_feedbuffer_tell;
		kradebml->io.userdata = kradebml;

		kradebml->info.cluster_at = cluster_at;
		 kradebml->info.userdata = kradebml;

  kradebml->ctx = ne_alloc(sizeof(*kradebml->ctx));

	if (kradebml->info.cluster_at != NULL) {
		printf("Got Info Callbacks\n");
		kradebml->ctx->info = &kradebml->info;
	}


  kradebml->ctx->io = ne_alloc(sizeof(*kradebml->ctx->io));
  *kradebml->ctx->io = kradebml->io;
  kradebml->ctx->log = log_callback;
  kradebml->ctx->alloc_pool = ne_pool_init();

  if (!kradebml->ctx->log)
    kradebml->ctx->log = ne_null_log_callback;


//	pthread_create(&kradebml->processing_thread, NULL, processing_thread, (void *)kradebml);


	return kradebml;

}

size_t kradebml_read_space(kradebml_t *kradebml) {

	size_t x;
	int space;
	
	//space = jack_ringbuffer_read_space(kradebml->output_ring);
	
	space = space - 8192;
	
	if (space < 0) {
		return 0;
	} else {
		x = space;
	}
	
	if ((kradebml->last_cluster_pos > kradebml->total_bytes_wrote) && (kradebml->last_cluster_pos < (kradebml->total_bytes_wrote + 4096))) {
		x = kradebml->last_cluster_pos - kradebml->total_bytes_wrote;
		printf("setting up cluster x-cross cluster at %lu doing %zu bytes\n", kradebml->last_cluster_pos, x);
		//exit(0);
		return x;
	}
	
	if (x > 4096) {
		return 4096;
	} else {
		return x;
	}



}


int kradebml_read(kradebml_t *kradebml, char *buffer, int len) {

	
	if ((kradebml->last_cluster_pos > kradebml->total_bytes_wrote) && (kradebml->last_cluster_pos < (kradebml->total_bytes_wrote + len))) {
		printf("Crossed cluster at %lu\n", kradebml->last_cluster_pos);
	}
	
	kradebml->total_bytes_wrote += len;
	
	if (kradebml->last_cluster_pos < kradebml->total_bytes_wrote) {
		printf("Bypassed cluster at %lu, currently at %lu\n", kradebml->last_cluster_pos, kradebml->total_bytes_wrote);
	}
	
	return 0;
	//return jack_ringbuffer_read(kradebml->output_ring, buffer, len);


}

int kradebml_last_was_sync(kradebml_t *kradebml) {


	if (kradebml->total_bytes_wrote == kradebml->last_cluster_pos) { 
		printf("last was synced!\n");
		return 1;
	
	} else {
	
		
		return 0;
	}

}


char *kradebml_write_buffer(kradebml_t *kradebml, int len) {

	kradebml->input_buffer = calloc(1, len);

	return kradebml->input_buffer;


}


int kradebml_wrote(kradebml_t *kradebml, int len) {



	//jack_ringbuffer_write(kradebml->input_ring, kradebml->input_buffer, len);
	//jack_ringbuffer_write(kradebml->output_ring, kradebml->input_buffer, len);

	free(kradebml->input_buffer);

	return 0;
}








int krad_ebml_find_first_cluster(char *buffer, int len) {

	uint64_t b;
	//uint64_t position;
	uint64_t found;
	uint64_t matched_byte_num;
	unsigned char match_byte;
	
	for (b = 0; b < len; b++) {
		if ((buffer[b] == match_byte) || (matched_byte_num > 0)) {
			match_byte = get_next_match_byte(buffer[b], b, &matched_byte_num, &found);
			if (found > 0) {
				//return (b - 4) + 1;
				return found;
			}
		}
	}
}







