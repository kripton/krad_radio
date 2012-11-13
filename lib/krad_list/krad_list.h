#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "krad_system.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define KRAD_URL "http://kradradio.com"
#define KRAD_EMAIL "kradradio@gmail.com"
#define KRAD_NAME "Krad Radio"
#define KRAD_IMAGE_URL "http://kradradio.com/image.jpg"
#define ITUNES_EXPLICIT "yes"

typedef enum {
	UNKNOWN,
	XSPF,
	PODCAST,
} krad_list_format_t;

typedef enum {
	NONE,
	DESCRIPTION,
	IMAGE,
	TITLE,
	LOCATION,
	IMAGETITLE,
	IMAGEURL,
	IGNOREIMAGEEXTRATAGS,
} krad_list_parse_state_t;

typedef struct {


	int ret;
	char buffer[4096];
	xmlParserCtxtPtr ctxt;
	int fd;
	va_list ap;
	xmlSAXHandler handler;
	int parsing_in_list;
	krad_list_format_t format;
	krad_list_parse_state_t parse_state;
	char error_message[2048];
	int char_pos;

	char title[1024];
	char image[1024];
	char imagetitle[1024];
	char imageurl[1024];
	int ignore_image;
	char description[2048];

	char url[2048];

	
	xmlDocPtr doc;
	xmlNodePtr root_node;
	xmlNodePtr node;
	xmlNodePtr list_node;
	xmlNodePtr stream_node;
	xmlNodePtr file_node;
	xmlAttrPtr prop;

	char *image_url;
	char *stream_node_name;
	
} krad_list_t;

krad_list_t *krad_list_open_file(char *filename);
void test_krad_list_open_file(int times);

krad_list_t *krad_list_create(krad_list_format_t format, char *title, char *description, char *image_url, char *creator);
void krad_list_add_item(krad_list_t *kradlist, char *title, char *link, char *description, char *date, char *image_url, char *size, char *type, char *duration);
void krad_list_save_file(krad_list_t *kradlist, char *filename);
void krad_list_destroy(krad_list_t *kradlist);

void krad_list_init();
void krad_list_shutdown();

void test_krad_list(int times);
