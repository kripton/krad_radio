#include <krad_ebml.h>

int main (int argc, char *argv[]) {

	krad_ebml_t *krad_ebml;	

	char *info;


	printf("Krad EBML %s\n", krad_ebml_version());

	if (argc != 2) {
		printf("Please specify a file\n");
		exit(1);
	}

	krad_ebml = krad_ebml_open_file(argv[1], KRAD_EBML_IO_READONLY);
	
	//info = kradebml_input_info(krad_ebml);
	
	//printf("%s\n", info); 

	krad_ebml_destroy(krad_ebml);

	return 0;

}
