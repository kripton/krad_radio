#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "krad_system.h"

#ifndef KRAD_TAG_H
#define KRAD_TAG_H

#define KRAD_MAX_TAGS 42

typedef struct krad_tag_St krad_tag_t;
typedef struct krad_tags_St krad_tags_t;

struct krad_tag_St {
	char *name;
	char *value;
};

struct krad_tags_St {
	krad_tag_t *tags;
	char *item;
	pthread_rwlock_t krad_tags_rwlock;
	
	void *callback_pointer;
	void (*set_tag_callback)( void *, char *, char *, char *, int);
	
};


void krad_tags_destroy (krad_tags_t *krad_tags);
krad_tags_t *krad_tags_create ();

void krad_tags_set_set_tag_callback (krad_tags_t *krad_tags, void *calllback_pointer, 
									 void (*set_tag_callback)( void *, char *, char *, char *, int));

char *krad_tags_get_tag (krad_tags_t *krad_tags, char *name);
void krad_tags_set_tag (krad_tags_t *krad_tags, char *name, char *value);

void krad_tags_set_tag_internal (krad_tags_t *krad_tags, char *name, char *value);
void krad_tags_set_tag_opt (krad_tags_t *krad_tags, char *name, char *value, int internal);

int krad_tags_get_next_tag (krad_tags_t *krad_tags, int *tagnum, char **name, char **value);

#endif
