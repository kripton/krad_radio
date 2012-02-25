#include "krad_ebml.h"


#define APPVERSION "KRAD EBML WRITE TEST"



int main (int argc, char *argv[]) {

	krad_ebml_t *krad_ebml;	
	char *filename;
	
	char *doctype;
	
	if (argc != 2) {
		printf("Please specify a file\n");
		exit(1);
	} else {
		filename = argv[1];
	}
	
	krad_ebml = krad_ebml_open_file(filename, KRAD_EBML_IO_WRITEONLY);
	
	doctype = "testdoc";
	
	krad_ebml_header (krad_ebml, doctype, APPVERSION);
	
	krad_ebml_write_tag (krad_ebml, "test tag 1", "monkey 123");

	krad_ebml_write_tag (krad_ebml, "test tag 2", "monkey 456");
	
	krad_ebml_destroy(krad_ebml);
	
	
	return 0;
	
}
