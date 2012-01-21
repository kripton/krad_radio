#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>

#define KRAD_EBML_HEADER_MAX_SIZE 8192 * 4
//#define KRAD_EBML_BUFFER_SIZE 8192
#define KRAD_EBML_BUFFER_SIZE 8192 * 512
//#define KRAD_EBML_READ_SIZE KRAD_EBML_BUFFER_SIZE
//#define KRAD_EBML_READ_SIZE (rand() % (KRAD_EBML_BUFFER_SIZE - 1)) + 1

#define KRAD_EBML_CLUSTER_BYTE1 0x1F
#define KRAD_EBML_CLUSTER_BYTE2 0x43
#define KRAD_EBML_CLUSTER_BYTE3 0xB6
#define KRAD_EBML_CLUSTER_BYTE4 0x75


unsigned char get_next_match_byte(unsigned char match_byte, uint64_t position, uint64_t *matched_byte_num, uint64_t *winner);

typedef struct kradebml_St kradebml_t;

struct kradebml_St {
	
	char cluster_mark[4];
	int cluster_count;

	int ret;
	int b;
	uint64_t position;
	uint64_t read_position;
	
	int buffer_position_internal;
	uint64_t cluster_position;
	int header_read;
	
	int header_size;
	int header_position;
	int header_read_position;

	unsigned char *input_buffer;
	unsigned char *buffer;
	unsigned char *header;

	uint64_t found;
	uint64_t matched_byte_num;
	unsigned char match_byte;
	int endcut;
	
	int last_was_cluster_end;
	int this_was_cluster_start;
	
};

void kradebml_destroy(kradebml_t *kradebml);
kradebml_t *kradebml_create_feedbuffer();
size_t kradebml_read_space(kradebml_t *kradebml);
int kradebml_read(kradebml_t *kradebml, char *buffer, int len);
int kradebml_last_was_sync(kradebml_t *kradebml);
char *kradebml_write_buffer(kradebml_t *kradebml, int len);
int kradebml_wrote(kradebml_t *kradebml, int len);
