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

void kradebml_destroy(kradebml_t *kradebml) {

	free(kradebml->buffer);
	free(kradebml->header);
	free(kradebml);

}

kradebml_t *kradebml_create_feedbuffer() {

	kradebml_t *kradebml = calloc(1, sizeof(kradebml_t));

	kradebml->buffer = calloc(1, KRAD_EBML_BUFFER_SIZE);
	kradebml->header = calloc(1, KRAD_EBML_HEADER_MAX_SIZE);

	
	kradebml->cluster_mark[0] = KRAD_EBML_CLUSTER_BYTE1;
	kradebml->cluster_mark[1] = KRAD_EBML_CLUSTER_BYTE2;
	kradebml->cluster_mark[2] = KRAD_EBML_CLUSTER_BYTE3;
	kradebml->cluster_mark[3] = KRAD_EBML_CLUSTER_BYTE4;

	return kradebml;

}

size_t kradebml_read_space(kradebml_t *kradebml) {

	size_t read_space;
	
	if (kradebml->header_read == 1) {
		read_space = (kradebml->position - kradebml->header_size) - kradebml->read_position;
	
		return read_space;
	} else {
		if (kradebml->header_size != 0) {
			return kradebml->header_size - kradebml->header_read_position;
		} else {
			return 0;
		}
	}


}

int kradebml_read(kradebml_t *kradebml, char *buffer, int len) {

	size_t read_space;
	size_t read_space_to_cluster;
	int to_read;
	
	read_space_to_cluster = 0;
	
	if (len < 1) {
		return 0;
	}
	
	if (kradebml->header_read == 1) {
		read_space = (kradebml->position - kradebml->header_size) - kradebml->read_position;
	
		if (read_space < 1) {
			return 0;
		}
		
		if (read_space >= len ) {
			to_read = len;
		} else {
			to_read = read_space;
		}
		
		if (kradebml->cluster_position != 0) {
			read_space_to_cluster = (kradebml->cluster_position - kradebml->header_size) - kradebml->read_position;
			if ((read_space_to_cluster != 0) && (read_space_to_cluster < to_read)) {
				to_read = read_space_to_cluster;
				kradebml->cluster_position = 0;
				kradebml->last_was_cluster_end = 1;
			}
		}
	
		memcpy(buffer, kradebml->buffer, to_read);
		kradebml->read_position += to_read;
		

		memmove(kradebml->buffer, kradebml->buffer + to_read, kradebml->buffer_position_internal - to_read);
		kradebml->buffer_position_internal -= to_read;
		
	} else {
		if (kradebml->header_size != 0) {
			
			read_space = kradebml->header_size - kradebml->header_read_position;
	
			if (read_space >= len ) {
				to_read = len;
			} else {
				to_read = read_space;
			}
	
			memcpy(buffer, kradebml->header, to_read);
			kradebml->header_read_position += to_read;		
			
			if (kradebml->header_read_position == kradebml->header_size) {
				kradebml->header_read = 1;
			}
			
		} else {
			return 0;
		}
	}
	
	
	return to_read;	

}

int kradebml_last_was_sync(kradebml_t *kradebml) {

	if (kradebml->last_was_cluster_end == 1) {
		kradebml->last_was_cluster_end = 0;
		kradebml->this_was_cluster_start = 1;
	}
	
	if (kradebml->this_was_cluster_start == 1) {
		kradebml->this_was_cluster_start = 0;
		return 1;
	}

	return 0;

}

char *kradebml_write_buffer(kradebml_t *kradebml, int len) {

	kradebml->input_buffer = malloc(len);

	return (char *)kradebml->input_buffer;


}


int kradebml_wrote(kradebml_t *kradebml, int len) {

	//start push
	
	//printf("\nwrote with %d bytes\n", len);
	
	int b;

	for (b = 0; b < len; b++) {
		if ((kradebml->input_buffer[b] == kradebml->match_byte) || (kradebml->matched_byte_num > 0)) {
			kradebml->match_byte = get_next_match_byte(kradebml->input_buffer[b], kradebml->position + b, &kradebml->matched_byte_num, &kradebml->found);
			if (kradebml->found > 0) {
				if (kradebml->header_size == 0) {
					if (b > 0) {
						memcpy(kradebml->header + kradebml->header_position, kradebml->input_buffer, b);
						kradebml->header_position += b;
					}
					kradebml->header_size = (kradebml->header_position - 4) + 1;
					printf("\ngot header, size is %d\n", kradebml->header_size);

					// first cluster
					memcpy(kradebml->buffer, kradebml->cluster_mark, 4);
					kradebml->buffer_position_internal += 4;
					if ((b + 1) < len) {
						memcpy(kradebml->buffer + kradebml->buffer_position_internal, kradebml->input_buffer + (b + 1), len - (b + 1));
						kradebml->buffer_position_internal += len - (b + 1);
					}
					
					printf("\nfound first cluster starting at %zu\n", kradebml->found);
					kradebml->cluster_position = kradebml->found;
					kradebml->position += len;
					free(kradebml->input_buffer);
					return len;

				}
				printf("\nfound cluster starting at %zu\n", kradebml->found);
				kradebml->cluster_position = kradebml->found;
			}
		}
	}
	
	if (kradebml->header_size == 0) {
		printf("\nadding to header header, pos is %d size is %d adding %d\n", kradebml->header_size, kradebml->header_position, len);
		memcpy(kradebml->header + kradebml->header_position, kradebml->input_buffer, len);
		kradebml->header_position += len;
	} else {
	
		memcpy(kradebml->buffer + kradebml->buffer_position_internal, kradebml->input_buffer, len);
		kradebml->buffer_position_internal += len;
	}
	
	kradebml->position += len;

	//end push

	free(kradebml->input_buffer);

	return len;
}
