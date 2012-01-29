#include <krad_ebml.h>

int main (int argc, char *argv[]) {

	krad_ebml_t *krad_ebml;	

	char *info;


	printf("Krad EBML %s\n", kradebml_version());

	if (argc != 2) {
		printf("Please specify a file\n");
		exit(1);
	}


	krad_ebml = kradebml_create();

	kradebml_open_input_file(krad_ebml, argv[1]);
	
	info = kradebml_input_info(krad_ebml);
	
	printf("%s\n", info); 

	kradebml_destroy(krad_ebml);

	return 0;

}
