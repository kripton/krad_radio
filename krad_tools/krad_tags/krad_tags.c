
#include <krad_tags.h>



void krad_tags_destroy (krad_tags_t *krad_tags) {

	int t;

	for (t = 0; t < KRAD_MAX_TAGS; t++) {
		if (krad_tags->tags[t].name != NULL) {
			free (krad_tags->tags[t].name);
			krad_tags->tags[t].name = NULL;
			free (krad_tags->tags[t].value);
			krad_tags->tags[t].value = NULL;
		}
	}

	free (krad_tags);

}

krad_tags_t *krad_tags_create () {

	krad_tags_t *krad_tags = calloc(1, sizeof(krad_tags_t));

	krad_tags->tags = calloc(KRAD_MAX_TAGS, sizeof(krad_tag_t));
	
	return krad_tags;

}


char *krad_tags_get_tag (krad_tags_t *krad_tags, char *name) {

	int t;

	for (t = 0; t < KRAD_MAX_TAGS; t++) {
		if (krad_tags->tags[t].name != NULL) {
			if (strcmp(krad_tags->tags[t].name, name) == 0) {
				return krad_tags->tags[t].value;
			}
		}
	}
	
	return "";

}

void krad_tags_set_tag (krad_tags_t *krad_tags, char *name, char *value) {

	int t;

	if ((name == NULL) || (strlen(name) == 0)) {
		return;
	}

	for (t = 0; t < KRAD_MAX_TAGS; t++) {
		if (krad_tags->tags[t].name != NULL) {
			if (strcmp(krad_tags->tags[t].name, name) == 0) {
				free (krad_tags->tags[t].value);
				krad_tags->tags[t].value = NULL;
				if ((value != NULL) && (strlen(value))) {
					krad_tags->tags[t].value = strdup (value);
				} else {
					free (krad_tags->tags[t].name);
					krad_tags->tags[t].name = NULL;
				}
				return;
			}
		}
	}
	
	if ((value == NULL) || (strlen(value) == 0)) {
		return;
	}
	
	for (t = 0; t < KRAD_MAX_TAGS; t++) {
		if (krad_tags->tags[t].name == NULL) {
			krad_tags->tags[t].name = strdup (name);
			krad_tags->tags[t].value = strdup (value);
			return;
		}
	}
}


int krad_tags_get_next_tag (krad_tags_t *krad_tags, int *tagnum, char **name, char **value) {

	int t;

	for (t = *tagnum; t < KRAD_MAX_TAGS; t++) {
		if (krad_tags->tags[t].name != NULL) {
			*name = krad_tags->tags[t].name;
			*value = krad_tags->tags[t].value;
			*tagnum = ++t;
			return 1;
		}
	}
	
	
	return 0;

}
