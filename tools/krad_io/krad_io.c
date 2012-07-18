#include "krad_io.h"


void krad_io_base64_encode(char *dest, char *src) {

	krad_io_base64_t *base64 = calloc(1, sizeof(krad_io_base64_t));

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




int krad_io_write(krad_io_t *krad_io, void *buffer, size_t length) {

//	if ((length + krad_io->write_buffer_pos) >= KRADio_WRITE_BUFFER_SIZE) {
//		krad_io_write_sync(krad_io);
//	}

	memcpy(krad_io->write_buffer + krad_io->write_buffer_pos, buffer, length);
	krad_io->write_buffer_pos += length;

	return length;
}

int krad_io_write_sync(krad_io_t *krad_io) {

	uint64_t length;
	
	length = krad_io->write_buffer_pos;
	krad_io->write_buffer_pos = 0;

	return krad_io->write(krad_io, krad_io->write_buffer, length);
}

int krad_io_read(krad_io_t *krad_io, void *buffer, size_t length) {
	return krad_io->read(krad_io, buffer, length);
}

int krad_io_seek(krad_io_t *krad_io, int64_t offset, int whence) {

	if (krad_io->mode == KRAD_IO_WRITEONLY) {
		if (whence == SEEK_CUR) {
			krad_io->write_buffer_pos += offset;
		}
		if (whence == SEEK_SET) {
			krad_io->write_buffer_pos = offset;
		}
		return krad_io->write_buffer_pos;
	}

	return krad_io->seek(krad_io, offset, whence);
}

int64_t krad_io_tell(krad_io_t *krad_io) {

	if (krad_io->mode == KRAD_IO_WRITEONLY) {
		return krad_io->write_buffer_pos;
	}

	return krad_io->tell(krad_io);
}

int krad_io_file_open(krad_io_t *krad_io) {

	int fd;
	
	fd = 0;
	
	if (krad_io->mode == KRAD_IO_READONLY) {
		fd = open ( krad_io->uri, O_RDONLY );
	}
	
	if (krad_io->mode == KRAD_IO_WRITEONLY) {
		fd = open ( krad_io->uri, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	}
	
	//printf("fd is %d\n", fd);
	
	krad_io->ptr = fd;

	return fd;
}

int krad_io_file_close(krad_io_t *krad_io) {
	return close(krad_io->ptr);
}

int krad_io_file_write(krad_io_t *krad_io, void *buffer, size_t length) {
	return write(krad_io->ptr, buffer, length);
}

int krad_io_file_read(krad_io_t *krad_io, void *buffer, size_t length) {

	//printf("len is %zu\n", length);

	return read(krad_io->ptr, buffer, length);

}

int64_t krad_io_file_seek(krad_io_t *krad_io, int64_t offset, int whence) {
	return lseek(krad_io->ptr, offset, whence);
}

int64_t krad_io_file_tell(krad_io_t *krad_io) {
	return lseek(krad_io->ptr, 0, SEEK_CUR);
}

int krad_io_stream_close(krad_io_t *krad_io) {
	return close(krad_io->sd);
}

int krad_io_stream_write(krad_io_t *krad_io, void *buffer, size_t length) {

	int bytes;
	
	bytes = 0;

	while (bytes != length) {

		bytes += send (krad_io->sd, buffer + bytes, length - bytes, 0);

		if (bytes <= 0) {
			failfast ("Krad io Source: send Got Disconnected from server");
		}
	}
	
	return bytes;
}


int krad_io_stream_read(krad_io_t *krad_io, void *buffer, size_t length) {

	int bytes;
	int total_bytes;

	total_bytes = 0;
	bytes = 0;

	while (total_bytes != length) {

		bytes = recv (krad_io->sd, buffer + total_bytes, length - total_bytes, 0);

		if (bytes <= 0) {
			if (bytes == 0) {
				printkd ("Krad IO Stream Recv: Got EOF\n");
				usleep (100000);
				return total_bytes;
			}
			if (bytes < 0) {
				printkd ("Krad IO Stream Recv: Got Disconnected\n");
				return total_bytes;
			}
		}
		
		total_bytes += bytes;
		
	}
	
	return total_bytes;
	
}


int krad_io_stream_open(krad_io_t *krad_io) {

	struct sockaddr_in serveraddr;
	struct hostent *hostp;
	char http_string[512];
	int http_string_pos;
	char content_type[64];
	char auth[256];
	char auth_base64[256];
	int sent;

	http_string_pos = 0;

	printkd ("Krad io: Connecting to %s:%d", krad_io->host, krad_io->port);

	if ((krad_io->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printkd ("Krad io Source: Socket Error");
	}

	memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(krad_io->port);
	
	if ((serveraddr.sin_addr.s_addr = inet_addr(krad_io->host)) == (unsigned long)INADDR_NONE)
	{
		// get host address 
		hostp = gethostbyname(krad_io->host);
		if(hostp == (struct hostent *)NULL)
		{
			printkd ("Krad io: Mount problem\n");
			close (krad_io->sd);
			exit (1);
		}
		memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
	}

	// connect() to server. 
	if((sent = connect(krad_io->sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
	{
		printkd ("Krad io Source: Connect Error");
	} else {


		if (krad_io->mode == KRAD_IO_READONLY) {
	
			sprintf (http_string, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", krad_io->mount, krad_io->host);
	
			printkd ("%s\n", http_string);
	
			krad_io_stream_write(krad_io, http_string, strlen(http_string));
	
			int end_http_headers = 0;
			char buf[8];
			while (end_http_headers != 4) {
	
				krad_io_stream_read(krad_io, buf, 1);
	
				printkd ("%c", buf[0]);
	
				if ((buf[0] == '\n') || (buf[0] == '\r')) {
					end_http_headers++;
				} else {
					end_http_headers = 0;
				}
	
			}
		} 
		
		if (krad_io->mode == KRAD_IO_WRITEONLY) {
		
		

			if ((strstr(krad_io->mount, ".ogg")) ||
				(strstr(krad_io->mount, ".opus")) ||
				(strstr(krad_io->mount, ".Opus")) ||
				(strstr(krad_io->mount, ".OPUS")) ||			
				(strstr(krad_io->mount, ".OGG")) ||
				(strstr(krad_io->mount, ".Ogg")) ||
				(strstr(krad_io->mount, ".oga")) ||
				(strstr(krad_io->mount, ".ogv")) ||
				(strstr(krad_io->mount, ".Oga")) ||		
				(strstr(krad_io->mount, ".OGV")))
			{
				strcpy(content_type, "application/ogg");
			} else {
				strcpy(content_type, "video/webm");
			}

			sprintf (auth, "source:%s", krad_io->password );
			krad_io_base64_encode( auth_base64, auth );
			http_string_pos = sprintf( http_string, "SOURCE %s ICE/1.0\r\n", krad_io->mount);
			http_string_pos += sprintf( http_string + http_string_pos, "content-type: %s\r\n", content_type);
			http_string_pos += sprintf( http_string + http_string_pos, "Authorization: Basic %s\r\n", auth_base64);
			http_string_pos += sprintf( http_string + http_string_pos, "\r\n");
			//printf("%s\n", http_string);
			krad_io_stream_write(krad_io, http_string, http_string_pos);
		
		}

	}

	return krad_io->sd;
}


void krad_io_destroy(krad_io_t *krad_io) {

	if (krad_io->mode == KRAD_IO_WRITEONLY) {
		krad_io_write_sync (krad_io);
	}

	if (krad_io->mode != -1) {
		krad_io->close(krad_io);
	}
		
	if (krad_io->write_buffer != NULL) {
		free (krad_io->write_buffer);
	}
	
	free (krad_io);

}

krad_io_t *krad_io_create() {

	krad_io_t *krad_io = calloc(1, sizeof(krad_io_t));

	return krad_io;

}

krad_io_t *krad_io_open_stream(char *host, int port, char *mount, char *password) {

	krad_io_t *krad_io;
	
	krad_io = krad_io_create();

	if (password == NULL) {
		krad_io->mode = KRAD_IO_READONLY;
	} else {
		krad_io->mode = KRAD_IO_WRITEONLY;
		krad_io->write_buffer = malloc(KRAD_IO_WRITE_BUFFER_SIZE);
	}
	//krad_io->seekable = 1;
	krad_io->read = krad_io_stream_read;
	krad_io->write = krad_io_stream_write;
	krad_io->open = krad_io_stream_open;
	krad_io->close = krad_io_stream_close;
	krad_io->uri = host;
	krad_io->host = host;
	krad_io->port = port;
	krad_io->mount = mount;
	krad_io->password = password;
	
	//krad_io->stream = 1;
	
	if (strcmp("ListenSD", krad_io->host) == 0) {
		krad_io->sd = krad_io->port;
	} else {
		krad_io->open (krad_io);
	}
		
	return krad_io;

}

krad_io_t *krad_io_open_file(char *filename, krad_io_mode_t mode) {

	krad_io_t *krad_io;
	
	krad_io = krad_io_create();

	krad_io->seek = krad_io_file_seek;
	krad_io->tell = krad_io_file_tell;
	krad_io->mode = mode;
	krad_io->seekable = 1;
	krad_io->read = krad_io_file_read;
	krad_io->write = krad_io_file_write;
	krad_io->open = krad_io_file_open;
	krad_io->close = krad_io_file_close;
	krad_io->uri = filename;
	krad_io->open(krad_io);

	if (krad_io->mode == KRAD_IO_WRITEONLY) {
		krad_io->write_buffer = malloc(KRAD_IO_WRITE_BUFFER_SIZE);
	}
	
	return krad_io;

} 
