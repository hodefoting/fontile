#!/bin/sh

PNG=MonoSans.asc
family=0xA000
variant=MonoSans
baselineoffset=0

rm -rf $family-$variant.ufo
./0xA000 $PNG $family-$variant $variant $baselineoffset 0
./fontconvert $family-$variant.ufo -t
