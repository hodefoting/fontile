all: tilefont-regular.ttf    \
		 tilefont-bold.ttf       \
		 tilefont-pixelated.ttf

CFLAGS += -O2 -g

fontile: fontile.c
	gcc $< -o $@ `pkg-config --cflags --libs glib-2.0`
%.ttf: %.asc fontile 5px-ascii.asc 
	./bake_ttf.sh `echo $< | sed s/\.asc//`
clean: 
	rm -rf *.ttf *.ufo
	rm -rf fontile
