# A8E

Fork from original A8E sources provided by the author Sacha Springer http://www.zerstoerung.de/


## To compile for mac:

### Pre-requisites:
```
brew install sdl
brew install sdl-image
```

### 1) Get SDL directories
```
xxxxx@TARDIS%  sdl-config --cflags --libs
output:
-I/usr/local/include/SDL -D_GNU_SOURCE=1 -D_THREAD_SAFE
-L/usr/local/lib -lSDLmain -lSDL -Wl,-framework,Cocoa
```

```
output
% /usr/local/Cellar/sdl2/2.0.14_1/bin/sdl2-config --cflags 
-I/usr/local/include/SDL2 -D_THREAD_SAFE
% /usr/local/Cellar/sdl2/2.0.14_1/bin/sdl2-config --libs
-L/usr/local/lib -lSDL2
```

### Modify the makefile

### Make the proyect

```
make -f Makefile
```
### Ejecute the program
./A8E

## To compile for web:

### Pre-requisites

- Install emscripten emsdk
- Source emsdk (`source emsdk_env.sh`)

### Make the project
```
make -f Makefile.emsdk
```

### Open an web server in the directory (ex. httplz)
```
http
```

### Open in the webbrowser
```
open http://localhost:8000/A8E.html
```
