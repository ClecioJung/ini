/* MIT License
 * 
 * Copyright (c) 2023 CLECIO JUNG <clecio.jung@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*------------------------------------------------------------------------------
 * HEADER
 *------------------------------------------------------------------------------
 */

#ifndef __INI_FILE
#define __INI_FILE

/* Summary:
 * INI files are not standardized, meaning that different implementations may have
 * differences. This implementation uses the # and ; characters to define
 * single-line comments. As a result, these characters cannot be used when defining
 * section names, keys, and values. Special characters such as =, #, and ; are not
 * allowed in key names. However, spaces and the = character can be used when
 * defining values, as long as the characters # and ; are not used. Section names
 * can have spaces, but cannot include the characters ], #, and ;. Nested sections
 * are not implemented. Duplicate section names are allowed and their key value
 * pairs are inserted under the same section data structure. Duplicate key names
 * results in error. Quoted strings and escaped characters are not supported in
 * this implementation.
 * If a key-value pair appears in the INI file before the first section is declared,
 * it will be treated as belonging to a global section which can be searched by
 * using NULL or empty strings for the section name field. This allows properties
 * to be defined outside of any specific section and still be easily accessible in
 * the program. 
 */

#include <stdio.h>
#include <stdlib.h>

/* This is a implementation of a custom string allocator to store the strings found
 * inside the INI. If you don't want to use this approach, just comment the
 * definition of the macro USE_CUSTOM_STRING_ALLOCATOR bellow. In this case, all the
 * string allocations will be performed by expensive malloc calls.
 * 
 * This custom string allocator consists of a linked-list of large buffers. It is
 * designed to be memory-efficient and avoid memory fragmentation. Whenever a new
 * string is found in the INI file, it is copied to an available buffer in the
 * linked list, allocating new buffers as needed. This approach reduces the number
 * of malloc and free calls, which can be expensive in terms of performance.
 */
#define USE_CUSTOM_STRING_ALLOCATOR
#ifdef USE_CUSTOM_STRING_ALLOCATOR

#define STRING_ALLOCATOR_BUFFER_SIZE 4096

struct String_Buffer {
    char buffer[STRING_ALLOCATOR_BUFFER_SIZE];
    struct String_Buffer *next;
};
#endif

typedef struct Key_Value_Pair {
    char *key;
    char *value;
} Key_Value_Pair;

typedef struct Ini_Section {
    char *name;
    /* The properties of the section are stored in a dynamic array */
    size_t properties_size;
    size_t properties_capacity;
    Key_Value_Pair *properties;
} Ini_Section;

typedef struct Ini_File {
#ifdef USE_CUSTOM_STRING_ALLOCATOR
    struct String_Buffer *strings;
    /* This index points to the next valid location in the buffer to store the string. */
    size_t string_index;
#endif
    /* The global section of the INI file. It's name is always empty */
    struct Ini_Section global_section;
    /* The sections of the ini file are stored in a dynamic array */
    size_t sections_size;
    size_t sections_capacity;
    Ini_Section *sections;
    /* Index of the section in which the properties should be inserted */
    Ini_Section *current_section;
} Ini_File;

typedef enum Ini_File_Error {
    ini_no_error = 0,
    ini_allocation,
    ini_invalid_parameters,
    ini_couldnt_open_file,
    ini_expected_closing_bracket,
    ini_expected_equals,
    ini_section_not_provided,
    ini_key_not_provided,
    ini_value_not_provided,
    ini_repeated_key,
    ini_no_such_section,
    ini_no_such_property,
    ini_not_integer,
    ini_not_unsigned,
    ini_not_double,

    NUMBER_OF_INI_FILE_ERRORS
} Ini_File_Error;

/* Callback used to handle errors and warnings in the parsing of INI files (function ini_file_parse).
 * In case of an error, this callback is called, and if it returns an integer different from zero,
 * we end the parsing and return NULL. */
typedef int (*Ini_File_Error_Callback)(const char *const filename, size_t line_number, size_t column, char *line, enum Ini_File_Error error);

size_t get_file_size(FILE *const file);
/* Remember to free the memory allocated for the returned string */
char *get_content_from_file(const char *const filename);

Ini_File *ini_file_new(void);
void ini_file_free(Ini_File *const ini_file);
void ini_section_print_to(const Ini_Section *const ini_section, FILE *const sink);
void ini_file_print_to(const Ini_File *const ini_file, FILE *const sink);
char *ini_file_error_to_string(const Ini_File_Error error);
/* This function is usefull for debug purposes */
void ini_file_info(const Ini_File *const ini_file);

/* Remember to free the memory allocated for the returned ini file structure */
Ini_File *ini_file_parse(const char *const filename, Ini_File_Error_Callback callback);

/* These functions use binary search algorithm to find the requested section and properties.
 * They return ini_no_error = 0 if everything worked correctly.
 * The found value will be stored at the memory address provided by the caller.
 * Note that the function may modify the value stored at the address provided even if the section/property isn't found. */
Ini_File_Error ini_file_find_section(Ini_File *const ini_file, const char *const section, Ini_Section **const ini_section);
Ini_File_Error ini_file_find_property(Ini_File *const ini_file, const char *const section, const char *const key, char **const value);
Ini_File_Error ini_file_find_integer(Ini_File *const ini_file, const char *const section, const char *const key, long *const integer);
Ini_File_Error ini_file_find_unsigned(Ini_File *const ini_file, const char *const section, const char *const key, unsigned long *const uint);
Ini_File_Error ini_file_find_double(Ini_File *const ini_file, const char *const section, const char *const key, double *const real);
Ini_File_Error ini_section_find_property(Ini_Section *const ini_section, const char *const key, char **const value);
Ini_File_Error ini_section_find_integer(Ini_Section *const ini_section, const char *const key, long *const integer);
Ini_File_Error ini_section_find_unsigned(Ini_Section *const ini_section, const char *const key, unsigned long *const uint);
Ini_File_Error ini_section_find_double(Ini_Section *const ini_section, const char *const key, double *const real);

/* These functions returns ini_no_error = 0 if everything worked correctly */
Ini_File_Error ini_file_add_section_sized(Ini_File *const ini_file, const char *const name, const size_t name_len);
Ini_File_Error ini_file_add_section(Ini_File *const ini_file, const char *const name);
Ini_File_Error ini_file_add_property_sized(Ini_File *const ini_file, const char *const key, const size_t key_len, const char *const value, const size_t value_len);
Ini_File_Error ini_file_add_property(Ini_File *const ini_file, const char *const key, const char *const value);
Ini_File_Error ini_file_save(const Ini_File *const ini_file, const char *const filename);

#endif  /* __INI_FILE */

/*------------------------------------------------------------------------------
 * END
 *------------------------------------------------------------------------------
 */

/* MIT License
 * 
 * Copyright (c) 2023 CLECIO JUNG <clecio.jung@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
