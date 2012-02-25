#include "krad2_ebml.h"


#define APPVERSION "KRAD2 EBML WRITE TEST"



int main (int argc, char *argv[]) {

	krad2_ebml_t *krad2_ebml;	
	char *filename;
	
	char *doctype;
	
	if (argc != 2) {
		printf("Please specify a file\n");
		exit(1);
	} else {
		filename = argv[1];
	}
	
	krad2_ebml = krad2_ebml_open_file(filename, KRAD2_EBML_IO_WRITEONLY);
	
	doctype = "testdoc";
	
	krad2_ebml_header (krad2_ebml, doctype, APPVERSION);
	
	
	krad2_ebml_destroy(krad2_ebml);
	
	
	return 0;
	
}
