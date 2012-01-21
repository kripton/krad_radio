#include <krad_ebml_simple.h>

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
			if ((read_space_to_cluster != 0) && (read_space_to_cluster < to_read)) {
				to_read = read_space_to_cluster;
				krad_ebml->cluster_position = 0;
				krad_ebml->last_was_cluster_end = 1;
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
