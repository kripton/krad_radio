#include "krad_system.h"

uint64_t re_fps (uint64_t frame_num, int input_numerator, int input_denominator, int output_numerator, int output_denominator) {

	uint64_t b;
	uint64_t c;	


	b = output_numerator * (uint64_t)input_denominator;
	c = input_numerator * (uint64_t)output_denominator;

	return ((frame_num * b) / c);


}


void convert_frame_nums (int num_frames, int input_numerator, int input_denominator, int output_numerator, int output_denominator) {

	uint64_t count;
	uint64_t converted_frame_num;

	count = 1;
	
	while (count != num_frames) {
		converted_frame_num = re_fps (count, input_numerator, input_denominator, output_numerator, output_denominator);
		printk ("Frame: %llu - %llu  (%llu/%llu -> %llu/%llu)", count, converted_frame_num,
				input_numerator, input_denominator, output_numerator, output_denominator);
		
		count++;
	}
}

int main (int argc, char *argv[]) {

	krad_system_init ();

	convert_frame_nums (60, 24000, 1000, 30000, 1000);

	convert_frame_nums (60, 30000, 1000, 24000, 1000);
	
	convert_frame_nums (60, 24000, 1000, 60000, 1000);

	convert_frame_nums (60, 60000, 1000, 30000, 1000);	
	
	printk ("Clean Exit");

	return 0;

}
