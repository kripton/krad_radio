#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
};


void krad_tags_destroy (krad_tags_t *krad_tags);
krad_tags_t *krad_tags_create ();

char *krad_tags_get_tag (krad_tags_t *krad_tags, char *name);
void krad_tags_set_tag (krad_tags_t *krad_tags, char *name, char *value);

int krad_tags_get_next_tag (krad_tags_t *krad_tags, int *tagnum, char **name, char **value);

#endif
