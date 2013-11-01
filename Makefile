all: tilefont-regular.ttf       \
		 tilefont-bold.ttf          \
		 tilefont-pixelated.ttf

CFLAGS += -O2 -g

0xA000: 0xA000.c
	gcc $< -o $@ `pkg-config --cflags --libs glib-2.0`
0xA000-cgen: 0xA000-cgen.c
	gcc $< -o $@ `pkg-config --cflags --libs gegl`

%.pal: %.png 0xA000-cgen
	./0xA000-cgen $< > $@
%.ttf: %.asc 0xA000 5px-ascii.asc 
	./bake_ttf.sh `echo $< | sed s/\.asc//`

%.html: %.content head.html
	cat head.html neck.html $< end.html > $@

# this also relies on all ufo dirs existing.
# it has to be manually invoked
Glyphs.content: Glyphs.content.sh UnicodeData.txt
	./$< > $@

# not including such a huge file in the repo..
UnicodeData.txt:
	wget ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt

clean: 
	rm -rf *.ttf *.ufo
	rm -rf *.pal
	rm -rf 0xA000 0xA000-cgen
