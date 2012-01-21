#include <krad_ebml_simple.h>

#define HEADER_MAX_SIZE 8192 * 4
#define BUFFER_SIZE 4096
#define READ_BUFFER_SIZE 8192
#define READ_SIZE BUFFER_SIZE
//#define READ_SIZE (rand() % (BUFFER_SIZE - 1)) + 1

void feed_test(char *filename, char *header_filename) {

	kradebml_t *kradebml;
	int fd, fd_h;
	uint64_t total_bytes_read;
	uint64_t total_bytes_wrote;
	int cluster_count;
	int wrote;
	char *inbuffer;
	char *outbuffer;
	char *writebuffer;
	int ret;
	size_t read_space;
	int to_read;
	int read_bytes;
	
	char *header;
	int header_size;
	
	int cluster_cut;
	
	header_size = 0;
	cluster_count = 0;
	total_bytes_read = 0;
	total_bytes_wrote = 0;
	
	cluster_cut = -1;
	
	inbuffer = calloc(1, BUFFER_SIZE);
	outbuffer = calloc(1, READ_BUFFER_SIZE);

	kradebml = kradebml_create_feedbuffer();

	fd = open(filename, O_RDONLY);

	if (fd < 1) {
		printf("error opening file: %s\n", filename);
		exit(1);
	}

	while ((ret = read(fd, inbuffer, READ_SIZE)) > 0) {

		writebuffer = kradebml_write_buffer(kradebml, ret);
		memcpy(writebuffer, inbuffer, ret);
		wrote = kradebml_wrote(kradebml, ret);
		
		if (wrote != ret) {
			printf("\nhrm wtf wrote\n");
		}
		
		total_bytes_wrote += wrote;
		
		read_space = kradebml_read_space(kradebml);

		//if (read_space >= READ_SIZE) {
		//	to_read = READ_SIZE;
		//} else {
			to_read = read_space;
		//}
		
		read_bytes = kradebml_read(kradebml, outbuffer, to_read);

		if (to_read != read_bytes) {
			printf("\nWanted to read %d bytes but read %d bytes\n", to_read, read_bytes);
		}
		
		if (header_size == 0) {
		
			header_size = read_bytes;
			header = malloc(header_size);
			memcpy(header, outbuffer, header_size);
			printf("got header! it was %d bytes\n", header_size);
			
			if ((header_filename != NULL) && (header_size > 0)) {
				printf("writing header of %d bytes to %s\n", header_size, header_filename);
				fd_h = open(header_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
				write(fd_h, header, header_size);
				//close(fd_h);
			}
			
		} else {
		
			if (((header_filename != NULL) && (header_size > 0)) && (cluster_count > cluster_cut)) {
				//printf("writing header of %d bytes to %s\n", header_size, header_filename);
				//fd_h = open(header_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
				write(fd_h, outbuffer, read_bytes);
				//close(fd_h);
			}
		
		}
		
		total_bytes_read += read_bytes;

		if (kradebml_last_was_sync(kradebml)) {
			cluster_count++;
		}

		printf("Total Bytes Read: %zu Wrote: %zu Clusters: %d\r", total_bytes_read, total_bytes_wrote, cluster_count);
		fflush(stdout);


	}
	
	
	printf("Total Bytes Read: %zu Wrote: %zu Clusters: %d\n", total_bytes_read, total_bytes_wrote, cluster_count);

	if ((header_filename != NULL) && (header_size > 0)) {
		close(fd_h);
	}
	
	free(header);
	free(inbuffer);
	free(outbuffer);
	close(fd);
	kradebml_destroy(kradebml);

}


int main (int argc, char *argv[]) {

	if (argc < 2) {
		printf("Please specify a file\n");
		exit(1);
	}
	
	if (argc == 2) {
		feed_test(argv[1], NULL);
	}

	if (argc == 3) {
		feed_test(argv[1], argv[2]);
	}

	return 0;

}
