#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define HEADER_MAX_SIZE 8192 * 4
#define BUFFER_SIZE 8192
//#define READ_SIZE BUFFER_SIZE
#define READ_SIZE (rand() % (BUFFER_SIZE - 1)) + 1



#define CLUSTER_BYTE1 0x1F
#define CLUSTER_BYTE2 0x43
#define CLUSTER_BYTE3 0xB6
#define CLUSTER_BYTE4 0x75

unsigned char get_next_match_byte(unsigned char match_byte, int position, int *matched_byte_num, int *winner) {

	if (winner != NULL) {
		*winner = 0;
	}

	if (matched_byte_num != NULL) {
		if (match_byte == CLUSTER_BYTE1) {
			if (matched_byte_num != NULL) {
				*matched_byte_num = position;
			}
			return CLUSTER_BYTE2;
		}
	
		if ((*matched_byte_num == position - 1) && (match_byte == CLUSTER_BYTE2)) {
			return CLUSTER_BYTE3;
		}
	
		if ((*matched_byte_num == position - 2) && (match_byte == CLUSTER_BYTE3)) {
			return CLUSTER_BYTE4;
		}	

		if ((*matched_byte_num == position - 3) && (match_byte == CLUSTER_BYTE4)) {
			if (winner != NULL) {
				*winner = *matched_byte_num;
			}
			*matched_byte_num = 0;
			return CLUSTER_BYTE1;
		}

		*matched_byte_num = 0;
	}
	
	return CLUSTER_BYTE1;

}

void find_clusters(char *filename) {

	int fd;
	int count;
	int ret;
	int b;
	int position;
	int last_cluster_position;
	//int cluster_size;
	unsigned char *buffer;


	int found;
	int matched_byte_num;
	unsigned char match_byte;


	found = 0;
	matched_byte_num = 0;
	last_cluster_position = 0;
	//header_size = 0;
	position = 0;
	count = 0;
	buffer = calloc(1, BUFFER_SIZE);

	fd = open(filename, O_RDONLY);

	if (fd < 1) {
		printf("error opening file: %s\n", filename);
		exit(1);
	}

	match_byte = get_next_match_byte(0x00, 0, NULL, NULL);

	while ((ret = read(fd, buffer, READ_SIZE)) > 0) {

		//printf("read %d bytes\n", ret);

		for (b = 0; b < ret; b++) {
			if ((buffer[b] == match_byte) || (matched_byte_num > 0)) {
				match_byte = get_next_match_byte(buffer[b], position + b, &matched_byte_num, &found);
				if (found > 0) {
					if (last_cluster_position != 0) {
						printf("size was %d\n\n", found - last_cluster_position);
					}
					printf("found cluster at %d\n", found);
					last_cluster_position = found;
					count++;
				}
			}
		}
		
		position += ret;

//		count++;
	}
	
	// this is wrong if anything is after last cluster (likely)
	if (last_cluster_position != 0) {
		printf("size was %d\n\n", position - last_cluster_position);
	}
	
	
	printf("Cluster count is %d\n", count);

	free(buffer);
	close(fd);



}


void dump_header(char *filename, char *header_filename) {

	int fd, fd_h;
	int count;
	int ret;
	int b;
	int position;
	int header_size;
	int header_position;
	int last_cluster_position;
	//int cluster_size;
	unsigned char *buffer;
	unsigned char *header;

	int found;
	int matched_byte_num;
	unsigned char match_byte;


	found = 0;
	matched_byte_num = 0;
	last_cluster_position = 0;
	header_size = 0;
	header_position = 0;
	position = 0;
	count = 0;
	buffer = calloc(1, BUFFER_SIZE);
	header = calloc(1, HEADER_MAX_SIZE);

	fd = open(filename, O_RDONLY);

	if (fd < 1) {
		printf("error opening file: %s\n", filename);
		exit(1);
	}

	match_byte = get_next_match_byte(0x00, 0, NULL, NULL);

	while ((ret = read(fd, buffer, READ_SIZE)) > 0) {

		//printf("read %d bytes\n", ret);

		for (b = 0; b < ret; b++) {
			if ((buffer[b] == match_byte) || (matched_byte_num > 0)) {
				match_byte = get_next_match_byte(buffer[b], position + b, &matched_byte_num, &found);
				if (found > 0) {
					if (last_cluster_position != 0) {
						printf("size was %d\n\n", found - last_cluster_position);
					}
					if (header_size == 0) {
						if (b > 0) {
							memcpy(header + header_position, buffer, b);
							header_position += b;
						}
						header_size = (header_position - 4) + 1;
						printf("got header, size is %d\n", header_size);
					}
					printf("found cluster at %d\n", found);
					last_cluster_position = found;
					count++;
				}
			}
		}
		
		if (header_size == 0) {
			memcpy(header + header_position, buffer, ret);
			header_position += ret;
		}
		
		position += ret;

//		count++;
	}
	
	// this is wrong if anything is after last cluster (likely)
	if (last_cluster_position != 0) {
		printf("size was %d\n\n", position - last_cluster_position);
	}
	
	
	printf("Cluster count is %d\n", count);


	fd_h = open(header_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	write(fd_h, header, header_size);
	close(fd_h);
	free(header);
	free(buffer);
	close(fd);



}


void cluster_cut(char *filename, char *output_filename, int cut) {

	char cluster_mark[4];
	
	cluster_mark[0] = CLUSTER_BYTE1;
	cluster_mark[1] = CLUSTER_BYTE2;
	cluster_mark[2] = CLUSTER_BYTE3;
	cluster_mark[3] = CLUSTER_BYTE4;

//	memcpy(cluster_mark, (char )0x1F43B675, 4);

	int fd, fd_o;
	int count;
	int ret;
	int b;
	int position;
	int header_size;
	int header_position;
	int last_cluster_position;
	int cluster_size;
	unsigned char *buffer;
	unsigned char *header;

	int found;
	int matched_byte_num;
	unsigned char match_byte;
	int endcut;


	endcut = 0;
	found = 0;
	matched_byte_num = 0;
	last_cluster_position = 0;
	header_size = 0;
	header_position = 0;
	position = 0;
	count = 0;
	buffer = calloc(1, BUFFER_SIZE);
	header = calloc(1, HEADER_MAX_SIZE);

	fd = open(filename, O_RDONLY);

	if (fd < 1) {
		printf("error opening input file: %s\n", filename);
		exit(1);
	}
	
	fd_o = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd_o < 1) {
		printf("error opening output file: %s\n", output_filename);
		exit(1);
	}
	
	printf("going to cut out the first %d clusters \n", cut);

	match_byte = get_next_match_byte(0x00, 0, NULL, NULL);

	while ((ret = read(fd, buffer, READ_SIZE)) > 0) {

		//start push

		for (b = 0; b < ret; b++) {
			if ((buffer[b] == match_byte) || (matched_byte_num > 0)) {
				match_byte = get_next_match_byte(buffer[b], position + b, &matched_byte_num, &found);
				if (found > 0) {
					if (last_cluster_position != 0) {
						printf("size was %d\n\n", found - last_cluster_position);
					}
					if (header_size == 0) {
						if (b > 0) {
							memcpy(header + header_position, buffer, b);
							header_position += b;
						}
						header_size = (header_position - 4) + 1;
						printf("got header, size is %d\n", header_size);
						write(fd_o, header, header_size);
					}
					printf("found cluster at %d\n", found);
					last_cluster_position = found;
					count++;
					if ((count > cut) && (endcut == 0)) {
						endcut = 1;
						//write(fd_o, buffer + (b - 3), ret - (b - 3));
						write(fd_o, cluster_mark, 4);
						if ((b + 1) < ret) {
							write(fd_o, buffer + (b + 1), ret - (b + 1));
						}
					}
				}
			}
		}
		
		if (endcut > 0) {
			if (endcut > 1) {
				write(fd_o, buffer, ret);
			}
			endcut = 2;
		}
		
		if (header_size == 0) {
			memcpy(header + header_position, buffer, ret);
			header_position += ret;
		}
		
		position += ret;

		//end push
	}
	
	// this is wrong if anything is after last cluster (likely)
	if (last_cluster_position != 0) {
		printf("size was %d\n\n", position - last_cluster_position);
	}
	
	
	printf("Cluster count is %d\n", count);

	close(fd_o);
	free(header);
	free(buffer);
	close(fd);



}


int main (int argc, char *argv[]) {

	if (argc < 2) {
		printf("Please specify a file\n");
		exit(1);
	}

	srand(time(NULL));

	if (argc == 2) {
		find_clusters(argv[1]);
	}

	if (argc == 3) {
		dump_header(argv[1], argv[2]);
	}
	
	if (argc == 4) {
		cluster_cut(argv[1], argv[2], atoi(argv[3]));
	}
	return 0;

}
