
/*------------------------------------------------------------------------------
 * SOURCE
 *------------------------------------------------------------------------------
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ini_file.h"

/* Most systems do not allow for a line greather than 4 kbytes */
#define MAX_LINE_SIZE 4096

#define INITIAL_SECTIONS_CAPACITY 32
#define INITIAL_PROPERTIES_CAPACITY 32

#ifdef USE_CUSTOM_STRING_ALLOCATOR
static void string_buffer_free(struct String_Buffer *buffer) {
    if (buffer == NULL) {
        return;
    }
    string_buffer_free(buffer->next);
    free(buffer);
}
#endif

size_t get_file_size(FILE *const file) {
    long file_size;
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return (size_t)file_size;
}

/* Remember to free the memory allocated for the returned string */
char *get_content_from_file(const char *const filename) {
	char *buffer = NULL;
	FILE *const file = fopen(filename, "rb");
	if (file != NULL) {
        const size_t file_size = get_file_size(file);
		/* Allocate memory to store the entire file */
		buffer = (char *)malloc((file_size + 1)*sizeof(char));
		if (buffer != NULL) {
			/* Copy the contents of the file to the buffer */
			const size_t result = fread(buffer, sizeof(char), file_size, file);
			buffer[file_size] = '\0';
			if (result != file_size) {
				/* Reading file error, free dinamically allocated memory */
				free(buffer);
				buffer = NULL;
			}
		}
		fclose(file);
	}
	return buffer;
}

struct Ini_File *ini_file_new(void) {
    struct Ini_File *ini_file = malloc(sizeof(struct Ini_File));
    if (ini_file == NULL) {
        return NULL;
    }
    memset(ini_file, 0, sizeof(struct Ini_File));
    ini_file->current_section = &ini_file->global_section;
    return ini_file;
}

static void ini_section_free(struct Ini_Section *const ini_section) {
#ifndef USE_CUSTOM_STRING_ALLOCATOR
    size_t i;
    free(ini_section->name);
    for (i = 0; i < ini_section->properties_size; i++) {
        free(ini_section->properties[i].key);
        free(ini_section->properties[i].value);
    }
#endif
    free(ini_section->properties);  
}

void ini_file_free(struct Ini_File *const ini_file) {
    size_t i;
    if (ini_file == NULL) {
        return;
    }
#ifdef USE_CUSTOM_STRING_ALLOCATOR
    string_buffer_free(ini_file->strings);
#endif
    for (i = 0; i < ini_file->sections_size; i++) {
        ini_section_free(&ini_file->sections[i]);
    }
    ini_section_free(&ini_file->global_section);
    free(ini_file->sections);
    free(ini_file);
}

void ini_section_print_to(const struct Ini_Section *const ini_section, FILE *const sink) {
    size_t property_index;
    if (ini_section == NULL) {
        return;
    }
    if ((ini_section->name != NULL) && (ini_section->name[0] != '\0')) {
        fprintf(sink, "[%s]\n", ini_section->name);
    }
    for (property_index = 0; property_index < ini_section->properties_size; property_index++) {
        fprintf(sink, "%s = %s\n", ini_section->properties[property_index].key, ini_section->properties[property_index].value);
    }
}

void ini_file_print_to(const struct Ini_File *const ini_file, FILE *const sink) {
    size_t section_index;
    if (ini_file == NULL) {
        return;
    }
    if (ini_file->global_section.properties_size > 0) {
        ini_section_print_to(&ini_file->global_section, sink);
        putchar('\n');
    }
    for (section_index = 0; section_index < ini_file->sections_size; section_index++) {
        ini_section_print_to(&ini_file->sections[section_index], sink);
        putchar('\n');
    }
}

char *ini_file_error_to_string(const Ini_File_Error error) {
    static char *const error_messages[] = {
        "No error has occured",
        "Couldn't allocate more memory",
        "Invalid parameters passed to the function",
        "Couldn't open file",
        "Expected closing square bracket ']'",
        "Expected equals sign '='",
        "A section name was not provided",
        "A key was not provided",
        "A value was not provided",
        "This key already exists",
        "Didn't found the requested section",
        "Didn't found the requested property",
        "The requested property is not a valid integer number",
        "The requested property is not a valid unsigned number",
        "The requested property is not a valid floating point number",
    };
#ifdef _Static_assert
    _Static_assert((NUMBER_OF_INI_FILE_ERRORS == (sizeof(error_messages)/sizeof(error_messages[0]))),
        "Don't forget to add the messages to this table");
#endif
    return error_messages[error];
}

/* This function is usefull for debug purposes */
void ini_file_info(const struct Ini_File *const ini_file) {
    size_t siz, i, allocs = 1, properties = 0, sections = ini_file->sections_size;
    if (ini_file == NULL) {
        return;
    }
    siz = sizeof(*ini_file) + sizeof(*ini_file->sections) * ini_file->sections_capacity;
    if (ini_file->global_section.properties_size > 0) {
#ifndef USE_CUSTOM_STRING_ALLOCATOR
        size_t j;
        for (j = 0; j < ini_file->global_section.properties_size; j++) {
            siz += 1 + strlen(ini_file->global_section.properties[j].key);
            siz += 1 + strlen(ini_file->global_section.properties[j].value);
        }
        allocs += 2 * ini_file->global_section.properties_size;
#endif
        properties += ini_file->global_section.properties_size;
        siz += sizeof(*ini_file->global_section.properties) * ini_file->global_section.properties_capacity;
        allocs++;
        sections++;
    }
    if (ini_file->sections_size > 0) {
        allocs++;
    }
#ifdef USE_CUSTOM_STRING_ALLOCATOR
    {
        struct String_Buffer *strings = ini_file->strings;
        while (strings != NULL) {
            siz += sizeof(*strings);
            strings = strings->next;
            allocs++;
        }
    }
#endif
    for (i = 0; i < ini_file->sections_size; i++) {
#ifndef USE_CUSTOM_STRING_ALLOCATOR
        size_t j;
        for (j = 0; j < ini_file->sections[i].properties_size; j++) {
            siz += 1 + strlen(ini_file->sections[i].properties[j].key);
            siz += 1 + strlen(ini_file->sections[i].properties[j].value);
        }
        siz += 1 + strlen(ini_file->sections[i].name);
        allocs += 1 + 2 * ini_file->sections[i].properties_size;
#endif
        properties += ini_file->sections[i].properties_size;
        siz += sizeof(*ini_file->sections[i].properties) * ini_file->sections[i].properties_capacity;
        if (ini_file->sections[i].properties_size > 0) {
            allocs++;
        }
    }
    printf("Sections:         %lu\n", sections);
    printf("Properties:       %lu\n", properties);
    printf("Allocated chunks: %lu\n", allocs);
    printf("Memory used:      %lu bytes\n", siz);
}

static char *copy_sized_string(struct Ini_File *ini_file, const char *const sized_str, const size_t len) {
    char *str;
#ifdef USE_CUSTOM_STRING_ALLOCATOR
    if (ini_file == NULL) {
        return NULL;
    }
    /* Checks if the string fits into the maximum buffer size */
    if ((len + 1) >= STRING_ALLOCATOR_BUFFER_SIZE) {
        return NULL;
    }
    if ((ini_file->strings == NULL) || ((ini_file->string_index + len + 1) > sizeof(ini_file->strings->buffer))) {
        /* Insert new buffer at the beginning */
        struct String_Buffer *new_strings = malloc(sizeof(struct String_Buffer));
        if (new_strings == NULL) {
            return NULL;
        }
        new_strings->next = ini_file->strings;
        ini_file->strings = new_strings;
        ini_file->string_index = 0;
    }
    /* Allocates the memory to store the string */
    str = &ini_file->strings->buffer[ini_file->string_index];
    ini_file->string_index += len + 1;
#else
    (void)ini_file;
    str = malloc(len + 1);
    if (str == NULL) {
        return NULL;
    }
#endif
    strncpy(str, sized_str, len);
    str[len] = '\0';
    return str;
}

static void advance_white_spaces(char **const str) {
    while (isspace((unsigned char) **str)) {
        (*str)++;
    }
}

static void advance_string_until(char **const str, const char *const chars) {
    while (strchr(chars, **str) == NULL) {
        (*str)++;
    }
}

/* This macro is used to simplify the error handling in the parser.
 * If a callback was provided, the error is reported to the user.
 * If the callback returns an integer different from zero,
 * we end the parsing and return NULL. */
#define ini_file_parse_handle_error(error) \
    do { \
        if (callback != NULL) { \
            if (callback(filename, line_number, (size_t)(cursor - line + 1), line, error) != 0) { \
                goto ini_file_parse_error; \
            } \
        } \
    } while (0)

/* Remember to free the memory allocated for the returned ini file structure */
struct Ini_File *ini_file_parse(const char *const filename, Ini_File_Error_Callback callback) {
    Ini_File_Error error;
    char line[MAX_LINE_SIZE];
    size_t line_number;
    FILE *file;
    struct Ini_File *ini_file = ini_file_new();
    if (ini_file == NULL) {
        /* This is a critical error, so we don't proceed, even if the callback returns 0 */
        if (callback != NULL) {
            callback(filename, 0, 0, NULL, ini_allocation);
        }
        return NULL;
    }
    file = fopen(filename, "rb");
	if (file == NULL) {
        /* This is a critical error, so we don't proceed, even if the callback returns 0 */
        if (callback != NULL) {
            callback(filename, 0, 0, NULL, ini_couldnt_open_file);
        }
        ini_file_free(ini_file);
        return NULL;
    }
    for (line_number = 1; fgets(line, sizeof(line), file) != NULL; line_number++) {
        char *cursor = line;
        char *key, *value;
        size_t key_len, value_len;
        advance_white_spaces(&cursor);
        /* Discards commments */
        if (strchr("#;", *cursor) != NULL) {
            continue;
        }
        /* Check if is a new section */
        if (*cursor == '[') {
            size_t name_len;
            char *name;
            cursor++;
            advance_white_spaces(&cursor);
            name = cursor;
            advance_string_until(&cursor, "]#;\r\n");
            if (*cursor != ']') {
                ini_file_parse_handle_error(ini_expected_closing_bracket);
                continue;
            }
            /* Compute length of the name string and remove trailing whitespaces */
            name_len = (size_t)(cursor - name);
            while ((name_len > 0) && (isspace((unsigned char)name[name_len - 1]))) {
                name_len--;
            }
            error = ini_file_add_section_sized(ini_file, name, name_len);
            if (error != ini_no_error) {
                ini_file_parse_handle_error(error);
            }
            /* We just ignore the possible characters after the end of the declaration of the section */
            continue;
        }
        key = cursor;
        advance_string_until(&cursor, "=#; \t\r\n");
        /* Compute length of the string name */
        key_len = (size_t)(cursor - key);
        if (key_len == 0) {
            ini_file_parse_handle_error(ini_key_not_provided);
            continue;
        }
        advance_white_spaces(&cursor);
        if (*cursor != '=') {
            ini_file_parse_handle_error(ini_expected_equals);
            continue;
        }
        cursor++;
        advance_white_spaces(&cursor);
        value = cursor;
        advance_string_until(&cursor, "#;\r\n");
        /* Compute length of the value string and remove trailing whitespaces */
        value_len = (size_t)(cursor - value);
        while ((value_len > 0) && (isspace((unsigned char)value[value_len - 1]))) {
            value_len--;
        }
        error = ini_file_add_property_sized(ini_file, key, key_len, value, value_len);
        if (error != ini_no_error) {
            ini_file_parse_handle_error(error);
        }
    }
    fclose(file);
    return ini_file;
ini_file_parse_error:
    ini_file_free(ini_file);
    fclose(file);
    return NULL;
}

/* This function compares a sized-string str1 with a null-terminated string str2 */
static int compare_sized_str_to_cstr(const char* str1, const char* str2, size_t len1) {
    const int comp = strncmp(str1, str2, len1);
    /* If str1 is equal to the first len characters of str2,
     * but str2 is longer than len characters, str1 is considered
     * less than str2 */
    if ((comp == 0) && (str2[len1] > '\0')) {
        return -1;
    }
    return comp;
}

/* Binary search algorithm */
#define binary_search(array, elem, str, len) \
    do { \
        size_t low = 0; \
        size_t high = array ## _size - 1; \
        while ((low <= high) && (high < array ## _size)) { \
            int comp; \
            *index = (low + high) / 2; \
            comp = compare_sized_str_to_cstr(str, array[*index].elem, len); \
            if (comp < 0) { \
                high = *index - 1; \
            } else if (comp > 0) { \
                low = *index + 1; \
            } else { \
                return ini_no_error; \
            } \
        } \
        /* Didn't found the requested element, so return the correct index \
         * to insert the new element, keeping the order of the array */ \
        *index = low; \
    } while (0)

static Ini_File_Error ini_file_find_section_index(struct Ini_File *const ini_file, const char *const section, const size_t section_len, size_t *const index) {
    binary_search(ini_file->sections, name, section, section_len);
    return ini_no_such_section;
}

static Ini_File_Error ini_file_find_key_index(struct Ini_Section *const ini_section, const char *const key, const size_t key_len, size_t *const index) {
    binary_search(ini_section->properties, key, key, key_len);
    return ini_no_such_property;
}

Ini_File_Error ini_file_find_section(struct Ini_File *const ini_file, const char *const section, Ini_Section **const ini_section) {
    Ini_File_Error  error;
    size_t section_index;
    if ((ini_file == NULL) || (ini_section == NULL)) {
        return ini_invalid_parameters;
    }
    if ((section == NULL) || (section[0] == '\0')) {
        *ini_section = &ini_file->global_section;
        return ini_no_error;
    }
    error = ini_file_find_section_index(ini_file, section, strlen(section), &section_index);
    if (error == ini_no_error) {
        *ini_section = &ini_file->sections[section_index];
    }
    return error;
}

Ini_File_Error ini_section_find_property(struct Ini_Section *const ini_section, const char *const key, char **const value)  {
    Ini_File_Error error;
    size_t property_index;
    if ((ini_section == NULL) || (value == NULL)) {
        return ini_invalid_parameters;
    }
    if (key[0] == '\0') {
        return ini_invalid_parameters;
    }
    error = ini_file_find_key_index(ini_section, key, strlen(key), &property_index);
    if (error == ini_no_error) {
        *value = ini_section->properties[property_index].value;
    }
    return error;
}

Ini_File_Error ini_file_find_property(struct Ini_File *const ini_file, const char *const section, const char *const key, char **const value) {
    Ini_File_Error error;
    struct Ini_Section *ini_section;
    if ((ini_file == NULL) || (key == NULL) || (value == NULL)) {
        return ini_invalid_parameters;
    }
    if (key[0] == '\0') {
        return ini_invalid_parameters;
    }
    error = ini_file_find_section(ini_file, section, &ini_section);
    if (error != ini_no_error) {
        return error;
    }
    return ini_section_find_property(ini_section, key, value);
}

static Ini_File_Error convert_to_integer(const char *const value, long *const integer) {
    char *end;
    long i_value = strtol(value, &end, 10);
    if (*end != '\0') {
        return ini_not_integer;
    }
    *integer = i_value;
    return ini_no_error;
}

Ini_File_Error ini_section_find_integer(struct Ini_Section *const ini_section, const char *const key, long *const integer) {
    char *value;
    Ini_File_Error error;
    if (integer == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_section_find_property(ini_section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    return convert_to_integer(value, integer);
}

Ini_File_Error ini_file_find_integer(struct Ini_File *const ini_file, const char *const section, const char *const key, long *const integer) {
    char *value;
    Ini_File_Error error;
    if (integer == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_file_find_property(ini_file, section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    return convert_to_integer(value, integer);
}

static Ini_File_Error convert_to_unsigned(const char *const value, unsigned long *const uint) {
    char *end;
    unsigned long ui_value = strtoul(value, &end, 10);
    if (*end != '\0') {
        return ini_not_unsigned;
    }
    *uint = ui_value;
    return ini_no_error;
}

Ini_File_Error ini_section_find_unsigned(struct Ini_Section *const ini_section, const char *const key, unsigned long *const uint) {
    char *value;
    Ini_File_Error error;
    if (uint == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_section_find_property(ini_section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    return convert_to_unsigned(value, uint);
}

Ini_File_Error ini_file_find_unsigned(struct Ini_File *const ini_file, const char *const section, const char *const key, unsigned long *const uint) {
    char *value;
    Ini_File_Error error;
    if (uint == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_file_find_property(ini_file, section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    return convert_to_unsigned(value, uint);
}

static Ini_File_Error convert_to_double(const char *const value, double *const real) {
    char *end;
    double d_value = strtod(value, &end);
    if (*end != '\0') {
        return ini_not_double;
    }
    *real = d_value;
    return ini_no_error;
}

Ini_File_Error ini_section_find_double(struct Ini_Section *const ini_section, const char *const key, double *const real)  {
    char *value;
    Ini_File_Error error;
    if (real == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_section_find_property(ini_section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    return convert_to_double(value, real);
}

Ini_File_Error ini_file_find_double(struct Ini_File *const ini_file, const char *const section, const char *const key, double *const real) {
    char *value;
    Ini_File_Error error;
    if (real == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_file_find_property(ini_file, section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    return convert_to_double(value, real);
}

static size_t max_size(const size_t a, const size_t b) {
    return ((a > b) ? a : b);
}

#define array_resize(array, default_cap) \
    do { \
        if ((array ## _size + 1) >= array ## _capacity) { \
            const size_t new_cap = max_size(2 * array ## _capacity, default_cap); \
            void *const new_array = realloc(array, new_cap * sizeof(*array)); \
            if (new_array == NULL) { \
                return ini_allocation; \
            } \
            array = new_array; \
            array ## _capacity = new_cap; \
        } \
    } while (0)

Ini_File_Error ini_file_add_section_sized(struct Ini_File *const ini_file, const char *const name, const size_t name_len) {
    size_t section_index;
    char *copied_name;
    if (ini_file == NULL) {
        return ini_invalid_parameters;
    }
    if ((name == NULL) || (name_len == 0)) {
        return ini_section_not_provided;
    }
    if (ini_file_find_section_index(ini_file, name, name_len, &section_index) == ini_no_error) {
        /* There is already a section with that name so we just update the current section */
        ini_file->current_section = &ini_file->sections[section_index];
        return ini_no_error;
    }
    /* Check if we need expand the array of sections */
    array_resize(ini_file->sections, INITIAL_SECTIONS_CAPACITY);
    /* Allocates memory to store the section name */
    copied_name = copy_sized_string(ini_file, name, name_len);
    if (copied_name == NULL) {
        return ini_allocation;
    }
    /* Updates the current section */
    ini_file->current_section = &ini_file->sections[section_index];
    /* Moves the sections to insert the new section in the middle, keeping the array sorted by names */
    memmove((ini_file->current_section + 1), ini_file->current_section, (ini_file->sections_size - section_index)*sizeof(struct Ini_Section));
    memset(ini_file->current_section, 0, sizeof(struct Ini_Section));
    ini_file->current_section->name = copied_name;
    ini_file->sections_size++;
    return ini_no_error;
}

Ini_File_Error ini_file_add_section(struct Ini_File *const ini_file, const char *const name) {
    if (name == NULL) {
        return ini_section_not_provided;
    }
    return ini_file_add_section_sized(ini_file, name, strlen(name));
}

Ini_File_Error ini_file_add_property_sized(struct Ini_File *const ini_file, const char *const key, const size_t key_len, const char *const value, const size_t value_len) {
    size_t property_index;
    struct Key_Value_Pair *property;
    char *copied_key, *copied_value;
    if (ini_file == NULL) {
        return ini_invalid_parameters;
    }
    if ((key == NULL) || (key_len == 0)) {
        return ini_key_not_provided;
    }
    if ((value == NULL) || (value_len == 0)) {
        return ini_value_not_provided;
    }
    if (ini_file_find_key_index(ini_file->current_section, key, key_len, &property_index) == ini_no_error) {
        /* There is already a property with that key name, which is not allowed */
        return ini_repeated_key;
    }
    /* Check if we need expand the array of properties */
    array_resize(ini_file->current_section->properties, INITIAL_PROPERTIES_CAPACITY);
    copied_key = copy_sized_string(ini_file, key, key_len);
    if (copied_key == NULL) {
        return ini_allocation;
    }
    copied_value = copy_sized_string(ini_file, value, value_len);
    if (copied_value == NULL) {
#ifndef USE_CUSTOM_STRING_ALLOCATOR
        free(copied_key);
#endif
        return ini_allocation;
    }
    property = &ini_file->current_section->properties[property_index];
    /* Moves the properties to insert the new property in the middle, keeping the array sorted by keys */
    memmove((property + 1), property, (ini_file->current_section->properties_size - property_index)*sizeof(struct Key_Value_Pair));
    /* Update the values to the new property */
    property->key = copied_key;
    property->value = copied_value;
    ini_file->current_section->properties_size++;
    return ini_no_error;
}

Ini_File_Error ini_file_add_property(struct Ini_File *const ini_file, const char *const key, const char *const value) {
    if (key == NULL) {
        return ini_key_not_provided;
    }
    if (value == NULL) {
        return ini_value_not_provided;
    }
    return ini_file_add_property_sized(ini_file, key, strlen(key), value, strlen(value));
}

Ini_File_Error ini_file_save(const struct Ini_File *const ini_file, const char *const filename) {
    FILE *file;
    if (ini_file == NULL) {
        return ini_invalid_parameters;
    }
    file = fopen(filename, "wb");
	if (file == NULL) {
        return ini_couldnt_open_file;
    }
    ini_file_print_to(ini_file, file);
    fclose(file);
    return ini_no_error;
}

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
