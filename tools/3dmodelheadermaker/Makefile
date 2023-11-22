MODELFILE:=../../main/modes/flight/3denv.h

all : $(MODELFILE)

objProcessor : objProcessor.c cnovrsupport.c
	gcc -o $@ $^ -lm

$(MODELFILE) : objProcessor 3denv.obj
	echo "#ifndef _MODEL3D_H" > $@
	echo "#define _MODEL3D_H" >> $@
	echo "" >> $@
	echo "const unsigned short model3d[] = {" >> $@
	./objProcessor 3denv.obj >> $@
	echo "};" >> $@
	echo "const unsigned short ship3d[] = {" >> $@
	./objProcessor ship2024.obj >> $@
	echo "};" >> $@
	echo "const unsigned short boolet3d[] = {" >> $@
	./objProcessor boolet.obj >> $@
	echo "};" >> $@
	echo "#endif" >> $@

clean :
	rm -rf *.o *~ objProcessor $MODELFILE
