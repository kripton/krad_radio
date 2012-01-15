gcc -g -Wall -c -fPIC krad_ebml.c  -ljack -I./halloc -o kradebml.o
gcc -g -Wall -c -fPIC halloc/src/halloc.c -I./halloc -I. -o halloc.o
gcc -Wall -fPIC -shared -Wl,-soname,libkradebml.so.1 -o libkradebml.so.1.0.1 kradebml.o halloc.o -ljack

gcc -g -Wall krad_ebml_test.c -lkradebml -I. -o krad_ebml_test

sudo cp krad_ebml.h /usr/local/include
sudo cp krad_ebml_ids.h /usr/local/include

sudo cp libkradebml.so.1.0.1 /usr/local/lib
sudo cp libkradebml.so.1.0.1 /usr/lib
sudo ldconfig
