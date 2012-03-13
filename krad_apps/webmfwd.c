#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>

#include "krad_ebml.h"

#define APPVERSION "Krad WebM FWD 0.3"

int do_shutdown;
int debug;
int verbose;

void help() {

	printf("%s\n", APPVERSION);
	
	printf("\nwebmfwd --password=PASSWORD [URL]\n\n");
	printf("Other options: \n");	
	printf("[--file filename] [--buffer seconds] [--verbose] [--debug]\n");
	printf("Default is to read from stdin and have a 7 second buffer.\n");
	printf("\noggfwd style options supported:\n");
	printf("webmfwd address port password mountpoint\n");	
	exit (0);

}

void dbg (char* format, ...) {

	va_list args;

	if (debug) {
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
	}
}

struct timespec timespec_diff(struct timespec start, struct timespec end) {

	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

void webmfwd_shutdown (int signal) {

	if (do_shutdown == 0) {
		printf("\nShutting down..\n");
		do_shutdown = 1;
	} else {
		printf("\nTerminating.\n");
		exit(1);
	}
	
}


int main (int argc, char *argv[]) {

	int o;
	int option_index;

	char *uri;
	
	int port;
	char host[512];
	char mount[512];
	char password[512];
	
	char input_filename[512];

	krad_ebml_t *input;
	krad_ebml_t *output;
	
	int buffer_seconds;
	
	unsigned char *buffer;
	int rd;
	int wr;	
	
	memset(input_filename, '\0', sizeof(input_filename));
	memset(host, '\0', sizeof(host));
	memset(mount, '\0', sizeof(mount));
	memset(password, '\0', sizeof(password));

	do_shutdown = 0;
	do_shutdown = 0;
	verbose = 0;
	debug = 0;

	port = 8000;	
	buffer_seconds = 7;

	while (1) {

		static struct option long_options[] = {
			{"file",			required_argument, 0, 'f'},
			{"debug",			no_argument, 0, 'd'},
			{"verbose",			no_argument, 0, 'v'},
			{"help",			no_argument, 0, 'h'},
			{"buffer",			required_argument, 0, 'b'},
			{"password",		required_argument, 0, 'p'},
			
			{0, 0, 0, 0}
		};

		option_index = 0;

		o = getopt_long ( argc, argv, "vhpfdb", long_options, &option_index );

		if (o == -1) {
			break;
		}

		switch ( o ) {
			case 'b':
				buffer_seconds = atoi(optarg);
				break;
			case 'f':
				strncpy (input_filename, optarg, sizeof(input_filename));
				break;
			case 'd':
				debug = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'p':
				strncpy(password, optarg, sizeof(password));
				break;
			case 'h':
				help ();
				break;
			default:
				abort ();
		}
	}

	if (optind < argc) {
	
		if ((argc - optind) == 4) {
			// oggfwd compatible options mode
			
			strncpy (host, argv[optind++], sizeof(host));
			port = atoi (argv[optind++]);
			strncpy (password, argv[optind++], sizeof(password));
			strncpy (mount, argv[optind], sizeof(mount));
			
			if (strchr(host, '.') == NULL) {
				printf("Invalid host: %s\n", host);
				help ();
			}
			
			if (!strlen(password)) {
				printf("Need password\n");
				help ();
			}
			
			if (!strlen(mount)) {
				printf("Missing mount point\n");
				help ();
			}
			
			if (strchr(mount, '/') == NULL) {
				printf("Invalid mount: %s\n", mount);
				help ();
			}
			
			
		} else {
	
		
			while (optind < argc) {
			
				if (!strlen(password)) {
					printf("Need password\n");
					help ();
				}
	
				if ((strlen(argv[optind]) > 7) && (strncmp(argv[optind], "http://", 7) == 0)) {
					uri = argv[optind] + 7;
				} else {
					uri = argv[optind];
				}
	
				if ((strchr(uri, '/')) && (strlen(uri) < sizeof(host))) {
					
					memcpy(host, uri, strcspn(uri, ":/"));
			
					if (strchr(host, '.') == NULL) {
						printf("Invalid host: %s\n", host);
						help ();
					}
		
					if (strchr(uri, ':') != NULL) {
						port = atoi(strchr(uri, ':') + 1);
					}
		
					memcpy(mount, strchr(uri, '/'), sizeof(mount));
		
				} else {
					printf("Missing mount point\n");
					exit(1);
				}
			
				optind++;
			}
		}
	} else {
		printf("Missing URL\n");
		help ();
	}

	signal(SIGTERM, webmfwd_shutdown);
	signal(SIGINT, webmfwd_shutdown);

	dbg ("Sending to %s:%d%s with password %s\n", host, port, mount, password);

	// this will read up until the end of the track info
	input = krad_ebml_open_file ( input_filename, KRAD_EBML_IO_READONLY );
	output = krad_ebml_open_stream(host, port, mount, password);

	// char *outfilename = "/home/oneman/kode/testfwdoutput.webm";
	// output = krad_ebml_open_file ( outfilename, KRAD_EBML_IO_WRITEONLY );
	
	// this will write out a generated header copying the needed webm information
	krad_ebml_header(output, "webm", APPVERSION);
	krad_ebml_add_video_track(output, "V_VP8", 0, input->tracks[1].width, input->tracks[1].height);		 
	krad_ebml_add_audio_track(output, "A_VORBIS", input->tracks[2].sample_rate, input->tracks[2].channels, input->tracks[2].codec_data, input->tracks[2].codec_data_size);
	krad_ebml_finish_element (output, output->tracks_info);
	krad_ebml_write_sync (output);
	
	buffer = malloc (1000000);
	
	/*
	// unthrottled
	while ((rd = krad_ebml_read(input, buffer, 4096)) > 0) {
		krad_ebml_write(output, buffer, rd);
		wr = krad_ebml_write_sync(output);
		if (wr != rd) {
			printf("Write error!\n");
			break;
		}
	}
	*/
	
	uint64_t packet_timecode;
	int current_track;
	int read_first_timecode;

	struct timespec start_time;
	struct timespec current_time;
	struct timespec elapsed_time;
	struct timespec input_time;
	struct timespec sleep_time;
	
	read_first_timecode = 0;
	
	krad_ebml_enable_read_copy ( input );
		
	clock_gettime (CLOCK_MONOTONIC, &start_time);
	
	while ((krad_ebml_read_packet ( input, &current_track, &packet_timecode, buffer) > 0) && (do_shutdown == 0)) {
	
		rd = krad_ebml_read_copy ( input , buffer );
		krad_ebml_write(output, buffer, rd);
		wr = krad_ebml_write_sync(output);
		if (wr != rd) {
			printf("Write error!\n");
			break;
		}
		
		if (read_first_timecode == 0) {
			start_time.tv_sec += packet_timecode / 1000;
			read_first_timecode = 1;
		}

		clock_gettime(CLOCK_MONOTONIC, &current_time);
		elapsed_time = timespec_diff (start_time, current_time);
		
		dbg ("input_timecode %ld \n", packet_timecode);

		input_time.tv_sec = packet_timecode / 1000;
		input_time.tv_nsec = (packet_timecode * 1000) - (packet_timecode % 1000);

		dbg ("elapsed_time %ld seconds and %ld nanoseconds\n", elapsed_time.tv_sec, elapsed_time.tv_nsec);
		dbg ("input_time %ld seconds and %ld nanoseconds\n", input_time.tv_sec, input_time.tv_nsec);

		if ((verbose) && (!debug)) {
			printf("Input Time: %ld Elapsed Time: %ld\r", input_time.tv_sec, elapsed_time.tv_sec);
			fflush (stdout);
		}

		input_time.tv_sec -= buffer_seconds;

		sleep_time = timespec_diff (elapsed_time, input_time);

		dbg ("sleep_time %ld seconds and %ld nanoseconds\n", sleep_time.tv_sec, sleep_time.tv_nsec);

		if ((sleep_time.tv_sec > -1) && (sleep_time.tv_nsec > 0))  {
			dbg ("sleeping for %ld seconds and %ld nanoseconds\n", sleep_time.tv_sec, sleep_time.tv_nsec);
			nanosleep (&sleep_time, NULL);
		}
	}

	free (buffer);
	
	krad_ebml_destroy ( input );
	krad_ebml_destroy ( output );
	
	dbg ("\nClean exit\n");
	
	return 0;

}
