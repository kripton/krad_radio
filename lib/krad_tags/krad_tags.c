
#include "krad_tags.h"



void krad_tags_destroy (krad_tags_t *krad_tags) {

	int t;
	pthread_rwlock_wrlock (&krad_tags->krad_tags_rwlock);
	for (t = 0; t < KRAD_MAX_TAGS; t++) {
		if (krad_tags->tags[t].name != NULL) {
			free (krad_tags->tags[t].name);
			krad_tags->tags[t].name = NULL;
			free (krad_tags->tags[t].value);
			krad_tags->tags[t].value = NULL;
		}
	}

	free (krad_tags->item);
	pthread_rwlock_unlock (&krad_tags->krad_tags_rwlock);
	pthread_rwlock_destroy (&krad_tags->krad_tags_rwlock);
	free (krad_tags);

}

krad_tags_t *krad_tags_create (char *item) {

	krad_tags_t *krad_tags = calloc(1, sizeof(krad_tags_t));

	krad_tags->tags = calloc(KRAD_MAX_TAGS, sizeof(krad_tag_t));
	
	if (krad_tags->tags == NULL) {
		failfast ("Krad Tags: Out of memory");
	}
	
	krad_tags->item = strdup (item);

	if (krad_tags->item == NULL) {
		failfast ("Krad Tags: Out of memory");
	}
	
	pthread_rwlock_init (&krad_tags->krad_tags_rwlock, NULL);
	
	return krad_tags;

}

void krad_tags_set_set_tag_callback (krad_tags_t *krad_tags, void *callback_pointer, 
									 void (*set_tag_callback)( void *, char *, char *, char *, int)) {


	krad_tags->callback_pointer = callback_pointer;
	krad_tags->set_tag_callback = set_tag_callback;

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
	krad_tags_set_tag_opt (krad_tags, name, value, 0);
}

void krad_tags_set_tag_internal (krad_tags_t *krad_tags, char *name, char *value) {
	krad_tags_set_tag_opt (krad_tags, name, value, 1);
}

void krad_tags_set_tag_opt (krad_tags_t *krad_tags, char *name, char *value, int internal) {

	int t;

	if ((name == NULL) || (strlen(name) == 0)) {
		return;
	}

	pthread_rwlock_wrlock (&krad_tags->krad_tags_rwlock);

	for (t = 0; t < KRAD_MAX_TAGS; t++) {
		if (krad_tags->tags[t].name != NULL) {
			if (strcmp(krad_tags->tags[t].name, name) == 0) {
				free (krad_tags->tags[t].value);
				krad_tags->tags[t].value = NULL;
				if ((value != NULL) && (strlen(value))) {
					krad_tags->tags[t].value = strdup (value);
					//UPDATED tag
					if (krad_tags->set_tag_callback) {
						krad_tags->set_tag_callback (krad_tags->callback_pointer, krad_tags->item, 
													 krad_tags->tags[t].name, krad_tags->tags[t].value, internal);
					}
				} else {
					//CLEARED tag
					if (krad_tags->set_tag_callback) {
						krad_tags->set_tag_callback (krad_tags->callback_pointer, krad_tags->item, 
													 krad_tags->tags[t].name, "", internal);
					}					
					free (krad_tags->tags[t].name);
					krad_tags->tags[t].name = NULL;
				}
				pthread_rwlock_unlock (&krad_tags->krad_tags_rwlock);				
				return;
			}
		}
	}
	
	if ((value == NULL) || (strlen(value) == 0)) {
		//wanted to create a tag with name but no value
		pthread_rwlock_unlock (&krad_tags->krad_tags_rwlock);	
		return;
	}
	
	for (t = 0; t < KRAD_MAX_TAGS; t++) {
		if (krad_tags->tags[t].name == NULL) {
			// NEW tag
			krad_tags->tags[t].name = strdup (name);
			krad_tags->tags[t].value = strdup (value);
			if (krad_tags->set_tag_callback) {
				krad_tags->set_tag_callback (krad_tags->callback_pointer, krad_tags->item, 
											 krad_tags->tags[t].name, krad_tags->tags[t].value, internal);
			}			
			pthread_rwlock_unlock (&krad_tags->krad_tags_rwlock);
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
