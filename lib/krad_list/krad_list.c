#include "krad_list.h"



void krad_list_parse_characters(krad_list_t *kradlist, const xmlChar *ch, int len) {

	if (!kradlist->parsing_in_list == 1) {
	
		if (kradlist->parse_state == IGNOREIMAGEEXTRATAGS) {
			return;
		}
	
	
		if (kradlist->parse_state == TITLE) {
			if (kradlist->char_pos > sizeof(kradlist->title)) {
				printke ("too long title!!\n");
				return;
			}
		
			memcpy(kradlist->title + kradlist->char_pos, ch, MIN((sizeof(kradlist->title) - kradlist->char_pos), len));
		
			kradlist->char_pos += len;
		
			if (kradlist->char_pos >= sizeof(kradlist->title)) {
				kradlist->title[sizeof(kradlist->title) - 1] = '\0';
				printke ("oh no too long title!!\n");
				return;
			}
		}
		
		if (kradlist->parse_state == IMAGETITLE) {
			if (kradlist->char_pos > sizeof(kradlist->imagetitle)) {
				printke ("too long imagetitle!!\n");
				return;
			}
		
			memcpy(kradlist->imagetitle + kradlist->char_pos, ch, MIN((sizeof(kradlist->imagetitle) - kradlist->char_pos), len));
		
			kradlist->char_pos += len;
		
			if (kradlist->char_pos >= sizeof(kradlist->imagetitle)) {
				kradlist->imagetitle[sizeof(kradlist->imagetitle) - 1] = '\0';
				printke ("oh no too long imagetitle!!\n");
				return;
			}
		}
		
		if (kradlist->parse_state == IMAGEURL) {
			if (kradlist->char_pos > sizeof(kradlist->imageurl)) {
				printke ("too long imageurl!!\n");
				return;
			}
		
			memcpy(kradlist->imageurl + kradlist->char_pos, ch, MIN((sizeof(kradlist->imageurl) - kradlist->char_pos), len));
		
			kradlist->char_pos += len;
		
			if (kradlist->char_pos >= sizeof(kradlist->imageurl)) {
				kradlist->imageurl[sizeof(kradlist->imageurl) - 1] = '\0';
				printke ("oh no too long imageurl!!\n");
				return;
			}
		}
		
		if (kradlist->parse_state == DESCRIPTION) {
			if (kradlist->char_pos > sizeof(kradlist->description)) {
				printke ("too long description!!\n");
				return;
			}
		
			memcpy(kradlist->description + kradlist->char_pos, ch, MIN((sizeof(kradlist->description) - kradlist->char_pos), len));
		
			kradlist->char_pos += len;
		
			if (kradlist->char_pos >= sizeof(kradlist->description)) {
				kradlist->description[sizeof(kradlist->description) - 1] = '\0';
				printke ("oh no too long description!!\n");
				return;
			}
		}
		
		if (kradlist->parse_state == IMAGE) {
		
			if (kradlist->char_pos > sizeof(kradlist->image)) {
				printke ("too long image!!\n");
				return;
			}
		
			memcpy(kradlist->image + kradlist->char_pos, ch, MIN((sizeof(kradlist->image) - kradlist->char_pos), len));
		
			kradlist->char_pos += len;
		
			if (kradlist->char_pos >= sizeof(kradlist->image)) {
				kradlist->image[sizeof(kradlist->image) - 1] = '\0';
				printke ("oh no too long image!!\n");
				return;
			}
		
		}
	
	} else {
	

		if (kradlist->parse_state == LOCATION) {
		
			if (kradlist->char_pos > sizeof(kradlist->url)) {
				printke ("too long url!!\n");
				return;
			}
		
			memcpy(kradlist->url + kradlist->char_pos, ch, MIN((sizeof(kradlist->url) - kradlist->char_pos), len));
		
			kradlist->char_pos += len;
		
			if (kradlist->char_pos >= sizeof(kradlist->url)) {
				kradlist->url[sizeof(kradlist->url) - 1] = '\0';
				printke ("oh no too long url!!\n");
				return;
			}
	
		}
	}

}

static void krad_list_parse_end_element (krad_list_t *kradlist, const xmlChar *name)
{

	if (kradlist->format == UNKNOWN) {
		return;
	}
	
	if ((kradlist->format == XSPF) || (kradlist->format == PODCAST)) {
		
		if (kradlist->parsing_in_list == 1) {
			if (xmlStrEqual (name, BAD_CAST "location")) {
				kradlist->url[kradlist->char_pos] = '\0';
				printkd ("Got Location: %s\n", kradlist->url);
		
				printkd ("Parse state now none\n");
				kradlist->parse_state = NONE;
				return;
			}
		} else {
			if (xmlStrEqual (name, BAD_CAST "title")) {

				if (kradlist->parse_state == IMAGETITLE) {
					kradlist->imagetitle[kradlist->char_pos] = '\0';
					printkd ("Got imagetitle: %s\n", kradlist->imagetitle);
		
					printkd ("Parse state now image\n");
					kradlist->char_pos = 0;
					kradlist->parse_state = IMAGE;
					return;
				} else {

					kradlist->title[kradlist->char_pos] = '\0';
					printkd ("Got title: %s\n", kradlist->title);
		
					printkd ("Parse state now none\n");
					kradlist->parse_state = NONE;
					return;
				}
			}

			if ((xmlStrEqual (name, BAD_CAST "description")) || (xmlStrEqual (name, BAD_CAST "annotation"))) {
				kradlist->description[kradlist->char_pos] = '\0';
				printkd ("Got description: %s\n", kradlist->description);
		
				printkd ("Parse state now none\n");
				kradlist->parse_state = NONE;
				return;
			}
		
			if (xmlStrEqual (name, BAD_CAST "image")) {
				kradlist->image[kradlist->char_pos] = '\0';
				
				if(kradlist->ignore_image == 1) {
					kradlist->char_pos = 0;
				}
				
				if (kradlist->char_pos > 0) {
					printkd ("char pos: %d\n", kradlist->char_pos);
					printkd ("Got image: %s\n", kradlist->image);
				}
				printkd ("Parse state now none\n");
				kradlist->parse_state = NONE;
				return;
			}
			
			if (xmlStrEqual (name, BAD_CAST "url")) {
				if (kradlist->parse_state == IMAGEURL) {
					kradlist->imageurl[kradlist->char_pos] = '\0';
					printkd ("Got imageurl: %s\n", kradlist->imageurl);
					kradlist->char_pos = 0;
					printkd ("Parse state now image\n");
					kradlist->parse_state = IMAGE;
					return;
				}
			}
			
			if (kradlist->parse_state == IGNOREIMAGEEXTRATAGS) {
				kradlist->parse_state = IMAGE;
				printkd ("Parse state now IMAGE\n");
				return;
			}
			
		}
	}


}

static void krad_list_parse_start_element (krad_list_t *kradlist, const xmlChar *name, const xmlChar **attrs)
{

	int i;
	
	//printk ("format %d\n", kradlist->format);
	//printk ("start elem %s\n", name);

	if (kradlist->format == UNKNOWN) {
		
		if (xmlStrEqual (name, BAD_CAST "rss")) {
			kradlist->format = PODCAST;
			printkd ("Got PODCAST!\n");
		}
		
		if (xmlStrEqual (name, BAD_CAST "playlist")) {
			kradlist->format = XSPF;
			printkd ("Got XSPF Playlist!\n");
		}

	}
	
	if (kradlist->format == UNKNOWN) {
		return;
	}
	
	if (kradlist->format == XSPF) {
		
		if (!kradlist->parsing_in_list) {
		
			if (xmlStrEqual (name, BAD_CAST "title")) {
				kradlist->char_pos = 0;
				kradlist->parse_state = TITLE;
				printkd ("Parse state now title\n");
		
			}
			
			if (xmlStrEqual (name, BAD_CAST "image")) {
				kradlist->char_pos = 0;
				kradlist->parse_state = IMAGE;
				printkd ("Parse state now image\n");
		
			}

			if (xmlStrEqual (name, BAD_CAST "annotation")) {
				kradlist->char_pos = 0;
				kradlist->parse_state = DESCRIPTION;
				printkd ("Parse state now DESCRIPTION/annotation\n");
		
			}
		
			if (xmlStrEqual (name, BAD_CAST "trackList")) {
		
				kradlist->parsing_in_list = 1;
				printkd ("Now parsing in list\n");
		
			}
		
		} else {
		
		
			if (xmlStrEqual (name, BAD_CAST "location")) {
		
				kradlist->parse_state = LOCATION;
				kradlist->char_pos = 0;
				printkd ("Parse state now LOCATION\n");
		
			}
		}
	}

	if (kradlist->format == PODCAST) {

		if (!kradlist->parsing_in_list) {
		
			if (xmlStrEqual (name, BAD_CAST "title")) {
		
				if (kradlist->parse_state == IMAGE) {
					kradlist->char_pos = 0;
					kradlist->parse_state = IMAGETITLE;
					printkd ("Parse state now IMAGETITLE\n");
				} else {
					kradlist->char_pos = 0;
					kradlist->parse_state = TITLE;
					printkd ("Parse state now title\n");
				}
				return;
		
			}
			
			if (xmlStrEqual (name, BAD_CAST "image")) {
				
				kradlist->char_pos = 0;
				kradlist->parse_state = IMAGE;
				printkd ("Parse state now image\n");
				return;
			}
			
			if (xmlStrEqual (name, BAD_CAST "url")) {
				
				if (kradlist->parse_state == IMAGE) {
					kradlist->char_pos = 0;
					kradlist->parse_state = IMAGEURL;
					printkd ("Parse state now IMAGEURL\n");
					kradlist->ignore_image = 1;
				} else {
					printkd ("unexpected url tag\n");
				}
				return;
			}

			if (xmlStrEqual (name, BAD_CAST "description")) {
				kradlist->char_pos = 0;
				kradlist->parse_state = DESCRIPTION;
				printkd ("Parse state now DESCRIPTION\n");
				return;
			}
			
			if (kradlist->parse_state == IMAGE) {
				kradlist->parse_state = IGNOREIMAGEEXTRATAGS;
				printkd ("Parse state now IGNOREIMAGEEXTRATAGS\n");
				return;
			}
			
		
			if (xmlStrEqual (name, BAD_CAST "item")) {
		
				kradlist->parsing_in_list = 1;
				printkd ("Now parsing in list\n");
		
			}
			
		} else {

			if (!attrs || !kradlist)
				return;

			if (!xmlStrEqual (name, BAD_CAST "enclosure"))
				return;

			for (i = 0; attrs[i]; i += 2) {
				char *attr;

				if (!xmlStrEqual (attrs[i], BAD_CAST "url"))
					continue;

				attr = (char *) attrs[i + 1];

				printkd ("Found %s\n", attr);

				break;
			}
		}
	
	}


}

static void krad_list_parse_error (krad_list_t *kradlist, const char *msg, ...)
{

	va_start (kradlist->ap, msg);
	vsnprintf (kradlist->error_message, sizeof (kradlist->error_message), msg, kradlist->ap);
	va_end (kradlist->ap);
	
	printke ("FAIL!! %s\n", kradlist->error_message);
}

krad_list_t *krad_list_open_file(char *filename) {

	krad_list_t *kradlist = calloc(1, sizeof(krad_list_t));

	kradlist->handler.characters = (charactersSAXFunc) krad_list_parse_characters;
	kradlist->handler.startElement = (startElementSAXFunc) krad_list_parse_start_element;
	kradlist->handler.endElement = (endElementSAXFunc) krad_list_parse_end_element;
	kradlist->handler.error = (errorSAXFunc) krad_list_parse_error;
	kradlist->handler.fatalError = (fatalErrorSAXFunc) krad_list_parse_error;

	if (filename[0] == '-') {
		if ((kradlist->fd = fileno(stdin)) == -1) {
			printke ("failed to open stdin");
		}
	} else {
		if ((kradlist->fd = open(filename, O_RDONLY)) == -1) {
			printke ("failed to open file");
		}
	}

	printke ("fd is %d\n", kradlist->fd);

    if (kradlist->fd > -1) {
        

		kradlist->ctxt = xmlCreatePushParserCtxt(&kradlist->handler, kradlist, kradlist->buffer, 0, NULL);

		if (!kradlist->ctxt) {
			printke ("Failed to create Context\n");
		} else {
		
	        while ((kradlist->ret = read(kradlist->fd, kradlist->buffer, sizeof(kradlist->buffer))) > 0) {
	            xmlParseChunk(kradlist->ctxt, kradlist->buffer, kradlist->ret, 0);
	        }
			
			close(kradlist->fd);
		    
		    xmlParseChunk(kradlist->ctxt, kradlist->buffer, 0, 1);
		    
		    //kradlist->doc = kradlist->ctxt->myDoc;
		    
		    xmlFreeParserCtxt(kradlist->ctxt);
		}

	}

	return kradlist;

}



krad_list_t *krad_list_create(krad_list_format_t format, char *title, char *description, char *image_url, char *creator) {

	krad_list_t *kradlist = calloc(1, sizeof(krad_list_t));

	kradlist->format = format;

	kradlist->doc = xmlNewDoc(BAD_CAST "1.0");


	if (kradlist->format == XSPF) {

		kradlist->root_node = xmlNewNode(NULL, BAD_CAST "playlist");
   	
	    xmlDocSetRootElement(kradlist->doc, kradlist->root_node);

		kradlist->stream_node_name = "track";
		xmlNewProp(kradlist->root_node, BAD_CAST "xmlns", BAD_CAST "http://xspf.org/ns/0/");
		xmlNewProp(kradlist->root_node, BAD_CAST "version", BAD_CAST "1");

    	xmlNewTextChild(kradlist->root_node, NULL, BAD_CAST "title", BAD_CAST title);
    	xmlNewTextChild(kradlist->root_node, NULL, BAD_CAST "creator", BAD_CAST creator);
    	if (description != NULL) {
	    	xmlNewTextChild(kradlist->root_node, NULL, BAD_CAST "annotation", BAD_CAST description);
    	}
    	
    	xmlNewTextChild(kradlist->root_node, NULL, BAD_CAST "info", BAD_CAST KRAD_URL);

    	if (image_url != NULL) {
			xmlNewTextChild(kradlist->root_node, NULL, BAD_CAST "image", BAD_CAST image_url);
		}

    	kradlist->list_node = xmlNewTextChild(kradlist->root_node, NULL, BAD_CAST "trackList", NULL);

	}
	
	if (kradlist->format == PODCAST) {

	    kradlist->root_node = xmlNewNode(NULL, BAD_CAST "rss");
   	
	    xmlDocSetRootElement(kradlist->doc, kradlist->root_node);

		kradlist->stream_node_name = "item";

		xmlNewProp(kradlist->root_node, BAD_CAST "xmlns:itunes", BAD_CAST "http://www.itunes.com/dtds/podcast-1.0.dtd");
		xmlNewProp(kradlist->root_node, BAD_CAST "version", BAD_CAST "2.0");

    	kradlist->list_node = xmlNewTextChild(kradlist->root_node, NULL, BAD_CAST "channel", NULL);

		// Needed
    	xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "title", BAD_CAST title);
    	xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "link", BAD_CAST KRAD_URL);
    	xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "description", BAD_CAST description);  
    	xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "language", BAD_CAST "en-us");

    	// Extra
    	//xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "lastBuildDate", BAD_CAST "Wed, 17 Jun 2013 14:00:00 GMT");
    	//xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "ttl", BAD_CAST "15");
    	xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "copyright", BAD_CAST "&#xA9; 2013 Krad Radio");
    	
    	// iTunes EXT
    	//xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:keywords", NULL);
    	xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:explicit", BAD_CAST ITUNES_EXPLICIT);
    	xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:subtitle", BAD_CAST description);
    	xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:author", BAD_CAST creator);
    	xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:summary", BAD_CAST description);

    	kradlist->node = xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:owner", NULL);
	    	xmlNewTextChild(kradlist->node, NULL, BAD_CAST "itunes:name", BAD_CAST creator);
	    	xmlNewTextChild(kradlist->node, NULL, BAD_CAST "itunes:email", BAD_CAST KRAD_EMAIL);

		kradlist->node = xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:image", NULL);
			kradlist->prop = xmlNewProp(kradlist->node, BAD_CAST "href", BAD_CAST image_url);
		/*			
    	kradlist->node = xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:category", BAD_CAST "");
			kradlist->prop = xmlNewProp(kradlist->node, BAD_CAST "text", BAD_CAST "");
    	kradlist->node = xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:category", BAD_CAST "");
			kradlist->prop = xmlNewProp(kradlist->node, BAD_CAST "text", BAD_CAST "");
    	kradlist->node = xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST "itunes:category", NULL);
			kradlist->prop = xmlNewProp(kradlist->node, BAD_CAST "text", BAD_CAST "");
		*/
	}

	return kradlist;

}


void krad_list_add_item(krad_list_t *kradlist, char *title, char *link, char *description, char *date, char *image_url, char *size, char *type, char *duration) {

	if ((kradlist->format == PODCAST) && (link == NULL)) {
		printke ("** UH OH, can't have podcast item with out a link!!\n");
		return;
	}

	kradlist->stream_node = xmlNewTextChild(kradlist->list_node, NULL, BAD_CAST kradlist->stream_node_name, NULL);

	xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "title", BAD_CAST title);

	if (kradlist->format == PODCAST) {

		xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "link", BAD_CAST KRAD_URL);
		xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "guid", BAD_CAST link);
		xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "pubDate", BAD_CAST date);		
		xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "description", BAD_CAST description);
	
		kradlist->node = xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "enclosure", NULL);
			xmlNewProp(kradlist->node, BAD_CAST "url", BAD_CAST link);

			if (size != NULL) {
				xmlNewProp(kradlist->node, BAD_CAST "length", BAD_CAST size);
			}
		
			if (type != NULL) {
				xmlNewProp(kradlist->node, BAD_CAST "type", BAD_CAST type);
			}
	
		xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "itunes:author", BAD_CAST KRAD_NAME);
		xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "itunes:subtitle", BAD_CAST description);
		xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "itunes:summary", BAD_CAST description);
	
		if (image_url != NULL) {
			kradlist->node = xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "itunes:image", NULL);
				xmlNewProp(kradlist->node, BAD_CAST "href", BAD_CAST image_url);
		}
		if (duration != NULL) {
			xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "itunes:duration", BAD_CAST duration);
		}
		//xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "itunes:keywords", BAD_CAST "");
		xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "itunes:explicit", BAD_CAST ITUNES_EXPLICIT);
		
	}
	
	if (kradlist->format == XSPF) {

		if (link != NULL) {
			xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "location", BAD_CAST link);
		}

		if (description != NULL) {
			xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "annotation", BAD_CAST description);
		}

		//xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "info", BAD_CAST KRAD_URL);

		if (image_url != NULL) {
			xmlNewTextChild(kradlist->stream_node, NULL, BAD_CAST "image", BAD_CAST image_url);
		}

	}
}


void krad_list_save_file(krad_list_t *kradlist, char *filename) {

	xmlSaveFormatFileEnc(filename, kradlist->doc, "UTF-8", 1);

}


void krad_list_destroy(krad_list_t *kradlist) {

	xmlFreeDoc(kradlist->doc);
	free(kradlist);

}

void krad_list_init() {

	LIBXML_TEST_VERSION

}


void krad_list_shutdown() {

    xmlCleanupParser();

}


void krad_list_test_open_file(int times) {

	krad_list_t *kradlist;
	
	char *testfile = "/home/supcom/test.xml";
	
	while (times != 0) {
	
		kradlist = krad_list_open_file(testfile);
		
		krad_list_destroy(kradlist);
		
		times--;
		
	}


}


void test_krad_list(int times) {

	krad_list_t *kradlist;
	krad_list_t *kradlist2;
	
	int i;
	
	
	while (times != 0) {
	
		i = 0;
	
		kradlist = krad_list_create(XSPF, "A Krad List", "Yay for it", KRAD_IMAGE_URL, "Krad Radio Person");
		kradlist2 = krad_list_create(PODCAST, "A Krad List", "Yay for it", KRAD_IMAGE_URL, "Krad Radio Person");

		for (i = 0; i < 3; i++) {
			krad_list_add_item(kradlist, "A Krad List Item", "http://kradradio.com:8080/radio1_128k.mp3", "A wonderful thing", "Wed, 11 Jun 2013 14:00:00 GMT", "http://kradradio.com/image.jpg", "1234567", "audio/x-m4a", "4:20");
			krad_list_add_item(kradlist2, "A Krad List Item", "http://kradradio.com:8080/radio1_128k.mp3", "A wonderful thing", "Wed, 11 Jun 2013 14:00:00 GMT", "http://kradradio.com/image.jpg", "1234567", "audio/x-m4a", "4:20");
		}

		//save_krad_list(kradlist, "-");
		//save_krad_list(kradlist2, "-");
	
		krad_list_destroy(kradlist);
		krad_list_destroy(kradlist2);
	
		times--;
		
	}
	
}
