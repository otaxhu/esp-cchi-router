# Router for ESP-IDF (esp_http_server)

## Installation:
1. Create a directory, and inside of that directory create a components directory:
```sh
$ mkdir project-name && mkdir project-name/components
$ cd project-name/components
```
2. Inside of the `components` directory, clone the repository:
```sh
$ git clone https://github.com/otaxhu/esp-cchi-router
```
3. Create the main component:
```sh
$ cd project-name
$ mkdir main && touch main/main.c main/CMakeLists.txt
```
4. Initialize the main's CMakeLists.txt file:
```cmake
# project-name/main/CMakeLists.txt
idf_component_register(SRCS "main.c"
                       REQUIRES esp_http_server
                                esp-cchi-router
)
```
4. Use the library functions provided, either in your main component or whatever component you want :D

You can seek for more documentation in the [header file](/include/esp_cchi.h)
