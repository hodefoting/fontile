all: fontile-regular.ttf   \
		 fontile-bold.ttf      \
		 fontile-pixelated.ttf

fontile: fontile.c
	gcc $< -g -O2 -o $@ `pkg-config --cflags --libs glib-2.0`
%.ttf: %.asc fontile *.asc
	./bake_ttf.sh `echo $< | sed s/\.asc//`
clean: 
	rm -rf *.ttf *.ufo
	rm -rf fontile
install:
	install fontile /usr/local/bin
