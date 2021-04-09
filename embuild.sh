#!/bin/bash
# Pre-requisites:
# Use bash
# Install emsdk
# source ../../emscripten/emsdk/
# pipenv shell
# do:
emcc 6502.c A8E.c AtariIo.c Antic.c Gtia.c Pia.c Pokey.c -DEMSCRIPTEN=1 -s TOTAL_MEMORY=167772160  -s ALLOW_MEMORY_GROWTH=1  -s LEGACY_GL_EMULATION=1  -s GL_UNSAFE_OPTS=0 --preload-file assets -o A8E.html
