#CC=g++
#SDK_PATH=../../include
#CFLAGS=-Wno-multichar -I $(SDK_PATH) -fno-rtti
#LDFLAGS=-lm -ldl -lpthread

#Capture: Capture.cpp $(SDK_PATH)/DeckLinkAPIDispatch.cpp
#	$(CC) -o Capture Capture.cpp $(SDK_PATH)/DeckLinkAPIDispatch.cpp $(CFLAGS) $(LDFLAGS)

rm *.o
rm *.a
rm *.so.*

# dynamic lib
g++ -Wall -fPIC -Wno-multichar -fno-rtti -c krad_decklink_capture.cpp vendor/DeckLinkAPIDispatch.cpp -I. -I./vendor
g++ -shared -Wl,-soname,libkrad_decklink_capture.so -o libkrad_decklink_capture.so *.o -lm -ldl -lpthread
gcc krad_decklink_test.c krad_decklink.c -o krad_decklink_test -I. -I./vendor -L. -lm -ldl -lpthread -lkrad_decklink_capture

rm *.o
rm *.a
rm *.so.*

#static

g++ -Wall -Wno-multichar -fno-rtti -c krad_decklink_capture.cpp vendor/DeckLinkAPIDispatch.cpp -I. -I./vendor
ar -cvq krad_decklink_capture.a *.o
g++ krad_decklink_test.c krad_decklink.c krad_decklink_capture.a -o krad_decklink_test_static -I. -I./vendor -L. -lm -ldl -lpthread

rm *.o
rm *.a
#rm *.so.*
