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

struct Key_Value_Pair {
    char *key;
    char *value;
};

struct Ini_Section {
    char *name;
    size_t properties_size;
    size_t properties_capacity;
    struct Key_Value_Pair *properties;
};

struct Ini_File {
    size_t sections_size;
    size_t sections_capacity;
    struct Ini_Section *sections;
};

enum Ini_File_Errors {
    ini_no_error = 0,
    ini_allocation,
    ini_invalid_parameters,
    ini_couldnt_open_file,
    ini_expected_clocing_bracket,
    ini_expected_equals,
    ini_section_not_provided,
    ini_key_not_provided,
    ini_value_not_provided,
    ini_no_such_section,
    ini_no_such_property,
    ini_not_integer,
    ini_not_float,

    NUMBER_OF_INI_FILE_ERRORS
};

/* Callback used to handle errors and warnings in the parsing of INI files (function ini_file_parse).
 * In case of an error, this callback is called, and if it returns an integer different from zero,
 * we end the parsing and return NULL. */
typedef int (*Ini_File_Error_Callback)(const char *const filename, const size_t line_number, const char *const line, const enum Ini_File_Errors error);

size_t get_file_size(FILE *const file);
/* Remember to free the memory allocated for the returned string */
char *get_content_from_file(const char *const filename);

struct Ini_File *ini_file_new(void);
void ini_file_free(struct Ini_File *const ini_file);
void ini_section_print_to(const struct Ini_Section *const ini_section, FILE *const sink);
void ini_file_print_to(const struct Ini_File *const ini_file, FILE *const sink);
char *ini_file_error_to_string(const enum Ini_File_Errors error);

/* Remember to free the memory allocated for the returned ini file structure */
struct Ini_File *ini_file_parse(const char *const filename, Ini_File_Error_Callback callback);

/* These functions returns ini_no_error = 0 if everything worked correctly */
enum Ini_File_Errors ini_file_add_section_sized(struct Ini_File *const ini_file, const char *const name, const size_t name_len);
enum Ini_File_Errors ini_file_add_section(struct Ini_File *const ini_file, const char *const name);
enum Ini_File_Errors ini_file_add_property_sized(struct Ini_File *const ini_file, const char *const key, const size_t key_len, const char *const value, const size_t value_len);
enum Ini_File_Errors ini_file_add_property(struct Ini_File *const ini_file, const char *const key, const char *const value);
enum Ini_File_Errors ini_file_save(const struct Ini_File *const ini_file, const char *const filename);

/* These functions use sequential search algorithm to find the requested section and properties */
/* These functions returns ini_no_error = 0 if everything worked correctly */
enum Ini_File_Errors ini_file_find_section(struct Ini_File *const ini_file, const char *const section, struct Ini_Section **ini_section);
enum Ini_File_Errors ini_file_find_property(struct Ini_File *const ini_file, const char *const section, const char *const key, char **value);
enum Ini_File_Errors ini_file_find_integer(struct Ini_File *const ini_file, const char *const section, const char *const key, long *integer);
enum Ini_File_Errors ini_file_find_float(struct Ini_File *const ini_file, const char *const section, const char *const key, double *real);

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
