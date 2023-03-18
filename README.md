# ini

## Overview

**ini** is a simple C library for parsing and creating [INI files](https://en.wikipedia.org/wiki/INI_file). It possesses the following characteristics:

- Compatible with C89
- Has no external dependencies

INI files are commonly used to store configuration data, and our library makes it easy to read and write these files in C programs.

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Examples](#examples)
- [License](#license)

## Installation

To use this library in your own project, simply download the source code from the repository and link the `ini.c` and `ini.h` files with your project.

## Usage

This library provides a set of functions for reading and writing INI files. Here's an example of how to read an INI file:

```c
#include <stdio.h>
#include <stdlib.h>
#include "ini_file.h"

int main(const int argc, const char **const argv) {
    struct Ini_File *ini_file;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s ini_file_name\n", argv[0]);
        return EXIT_FAILURE;
    }
    ini_file = ini_file_parse(argv[1], NULL);
    if (ini_file == NULL) {
        fprintf(stderr, "Was not possible to parse the ini_file \"%s\"\n", argv[1]);
        return EXIT_FAILURE;
    }
    printf("\nThe properties retrieved from the the ini file \"%s\" are:\n\n", argv[1]);
    ini_file_print_to(ini_file, stdout);
    ini_file_free(ini_file);
    return EXIT_SUCCESS;
}
```

For a complete list of functions and their documentation, see the `ini.h` header file.

## Examples

In the examples folder, you can find complete examples of how to use the library. To compile them, simply type `make` at your terminal. Run the executables and follow the instructions provided.

## License

This project is released under the MIT license. See the LICENSE file for more information.
