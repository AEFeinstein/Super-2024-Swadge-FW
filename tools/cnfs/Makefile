all : cnfs_gen image.c

cnfs_gen : cnfs_gen.c
	gcc -o $@ $^ -Og -g

image.c : cnfs_gen
	./cnfs_gen ../../spiffs_image image.c image.h

clean :
	rm -rf image.c cnfs_gen image.h

