
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
    /* TODO: If we sort the sections, we shall think in another way to store
     * and retrieve the data of this global section */
    /* The first section is global to the file */
    if (ini_file_add_section(ini_file, "global") != 0) {
        ini_file_free(ini_file);
        return NULL;
    }
    return ini_file;
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
#ifndef USE_CUSTOM_STRING_ALLOCATOR
        size_t j;
        free(ini_file->sections[i].name);
        for (j = 0; j < ini_file->sections[i].properties_size; j++) {
            free(ini_file->sections[i].properties[j].key);
            free(ini_file->sections[i].properties[j].value);
        }
#endif
        free(ini_file->sections[i].properties);
    }
    free(ini_file->sections);
    free(ini_file);
}

void ini_section_print_to(const struct Ini_Section *const ini_section, FILE *const sink) {
    size_t property_index;
    if (ini_section == NULL) {
        return;
    }
    fprintf(sink, "[%s]\n", ini_section->name);
    for (property_index = 0; property_index < ini_section->properties_size; property_index++) {
        fprintf(sink, "%s = %s\n", ini_section->properties[property_index].key, ini_section->properties[property_index].value);
    }
}

void ini_file_print_to(const struct Ini_File *const ini_file, FILE *const sink) {
    size_t section_index, property_index;
    if (ini_file == NULL) {
        return;
    }
    for (section_index = 0; section_index < ini_file->sections_size; section_index++) {
        if (section_index != 0) {
            fprintf(sink, "[%s]\n", ini_file->sections[section_index].name);
        }
        for (property_index = 0; property_index < ini_file->sections[section_index].properties_size; property_index++) {
            fprintf(sink, "%s = %s\n",
                ini_file->sections[section_index].properties[property_index].key,
                ini_file->sections[section_index].properties[property_index].value);
        }
        putchar('\n');
    }
}

char *ini_file_error_to_string(const enum Ini_File_Errors error) {
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
    size_t siz, i, allocs = 1, properties = 0;
    if (ini_file == NULL) {
        return;
    }
    siz = sizeof(*ini_file) + sizeof(*ini_file->sections) * ini_file->sections_capacity;
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
    printf("Sections:         %lu\n", ini_file->sections_size);
    printf("Properties:       %lu\n", properties);
    printf("Allocated chunks: %lu\n", allocs);
    printf("Memory used:      %lu bytes\n", siz);
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

static char *copy_sized_string(struct Ini_File *ini_file, const char *const sized_str, const size_t len) {
    char *str;
#ifdef USE_CUSTOM_STRING_ALLOCATOR
    if (ini_file == NULL) {
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

static int ini_file_parse_handle_error(Ini_File_Error_Callback callback, const char *const filename, const size_t line_number, const size_t column, const char *const line, const enum Ini_File_Errors error) {
    /* This function is called when we found an error in the parsing.
     * So, we report it to the user using the callback provided. 
     * If the callback returns an integer different from zero,
     * we end the parsing and return NULL. */
    if (callback != NULL) {
        return callback(filename, line_number, column, line, error);
    }
    return 0;
}

/* TODO: Check for repeated section and key names? */
/* TODO: Sort the sections and keys? This would allow us to use binary search */

/* Remember to free the memory allocated for the returned ini file structure */
struct Ini_File *ini_file_parse(const char *const filename, Ini_File_Error_Callback callback) {
    enum Ini_File_Errors error;
    char line[MAX_LINE_SIZE];
    size_t line_number;
    FILE *file;
    struct Ini_File *ini_file = ini_file_new();
    if (ini_file == NULL) {
        /* This is a critical error, so we don't proceed, even if the callback returns 0 */
        ini_file_parse_handle_error(callback, filename, 0, 0, NULL, ini_allocation);
        return NULL;
    }
    file = fopen(filename, "rb");
	if (file == NULL) {
        /* This is a critical error, so we don't proceed, even if the callback returns 0 */
        ini_file_parse_handle_error(callback, filename, 0, 0, NULL, ini_couldnt_open_file);
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
                if (ini_file_parse_handle_error(callback, filename, line_number, (size_t)(cursor-line+1), line, ini_expected_clocing_bracket) != 0) {
                    goto ini_file_parse_error;
                }
                continue;
            }
            /* Compute length of the name string and remove trailing whitespaces */
            name_len = (size_t)(cursor - name);
            while ((name_len > 0) && (isspace((unsigned char)name[name_len - 1]))) {
                name_len--;
            }
            error = ini_file_add_section_sized(ini_file, name, name_len);
            if (error != ini_no_error) {
                if (ini_file_parse_handle_error(callback, filename, line_number, (size_t)(cursor-line+1), line, error) != 0) {
                    goto ini_file_parse_error;
                }
            }
            /* We just ignore the possible characters after the end of the declaration of the section */
            continue;
        }
        key = cursor;
        advance_string_until(&cursor, "=#; \t\r\n");
        /* Compute length of the string name */
        key_len = (size_t)(cursor - key);
        if (key_len == 0) {
            if (ini_file_parse_handle_error(callback, filename, line_number, (size_t)(cursor-line+1), line, ini_key_not_provided) != 0) {
                goto ini_file_parse_error;
            }
            continue;
        }
        advance_white_spaces(&cursor);
        if (*cursor != '=') {
            if (ini_file_parse_handle_error(callback, filename, line_number, (size_t)(cursor-line+1), line, ini_expected_equals) != 0) {
                goto ini_file_parse_error;
            }
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
            if (ini_file_parse_handle_error(callback, filename, line_number, (size_t)(cursor-line+1), line, error) != 0) {
                goto ini_file_parse_error;
            }
        }
    }
    fclose(file);
    return ini_file;
ini_file_parse_error:
    ini_file_free(ini_file);
    fclose(file);
    return NULL;
}

static size_t max_size(const size_t a, const size_t b) {
    return ((a > b) ? a : b);
}

enum Ini_File_Errors ini_file_add_section_sized(struct Ini_File *const ini_file, const char *const name, const size_t name_len) {
    struct Ini_Section *ini_section;
    if (ini_file == NULL) {
        return ini_invalid_parameters;
    }
    if ((name == NULL) || (name_len == 0)) {
        return ini_section_not_provided;
    }
    /* Check if we need expand our array of sections */
    if ((ini_file->sections_size + 1) >= ini_file->sections_capacity) {
        const size_t new_cap = max_size(2 * ini_file->sections_capacity, INITIAL_SECTIONS_CAPACITY);
        struct Ini_Section *const new_sections = realloc(ini_file->sections, new_cap * sizeof(struct Ini_Section));
        if (new_sections == NULL) {
            return ini_allocation;
        }
        ini_file->sections = new_sections;
        ini_file->sections_capacity = new_cap;
    }
    /* Insert a new section at the end of the array */
    ini_section = &ini_file->sections[ini_file->sections_size];
    memset(ini_section, 0, sizeof(struct Ini_Section));
    ini_section->name = copy_sized_string(ini_file, name, name_len);
    if (ini_section->name == NULL) {
        return ini_allocation;
    }
    ini_file->sections_size++;
    return ini_no_error;
}

enum Ini_File_Errors ini_file_add_section(struct Ini_File *const ini_file, const char *const name) {
    if (name == NULL) {
        return ini_section_not_provided;
    }
    return ini_file_add_section_sized(ini_file, name, strlen(name));
}

enum Ini_File_Errors ini_file_add_property_sized(struct Ini_File *const ini_file, const char *const key, const size_t key_len, const char *const value, const size_t value_len) {
    struct Ini_Section *ini_section;
    struct Key_Value_Pair *property;
    if (ini_file == NULL) {
        return ini_invalid_parameters;
    }
    if ((key == NULL) || (key_len == 0)) {
        return ini_key_not_provided;
    }
    if ((value == NULL) || (value_len == 0)) {
        return ini_value_not_provided;
    }
    if (ini_file->sections_size == 0) {
        return ini_allocation;
    }
    /* Insert the new property at the last section */
    ini_section = &ini_file->sections[ini_file->sections_size - 1];
    if ((ini_section->properties_size + 1) >= ini_section->properties_capacity) {
        const size_t new_cap = max_size(2 * ini_section->properties_capacity, INITIAL_PROPERTIES_CAPACITY);
        struct Key_Value_Pair *const new_properties = realloc(ini_section->properties, new_cap * sizeof(struct Key_Value_Pair));
        if (new_properties == NULL) {
            return ini_allocation;
        }
        ini_section->properties = new_properties;
        ini_section->properties_capacity = new_cap;
    }
    property = &ini_section->properties[ini_section->properties_size];
    property->key = copy_sized_string(ini_file, key, key_len);
    if (property->key == NULL) {
        return ini_allocation;
    }
    property->value = copy_sized_string(ini_file, value, value_len);
    if (property->value == NULL) {
#ifndef USE_CUSTOM_STRING_ALLOCATOR
        free(property->key);
#endif
        return ini_allocation;
    }
    ini_section->properties_size++;
    return ini_no_error;
}

enum Ini_File_Errors ini_file_add_property(struct Ini_File *const ini_file, const char *const key, const char *const value) {
    if (key == NULL) {
        return ini_key_not_provided;
    }
    if (value == NULL) {
        return ini_value_not_provided;
    }
    return ini_file_add_property_sized(ini_file, key, strlen(key), value, strlen(value));
}

enum Ini_File_Errors ini_file_save(const struct Ini_File *const ini_file, const char *const filename) {
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

enum Ini_File_Errors ini_file_find_section(struct Ini_File *const ini_file, const char *const section, struct Ini_Section **ini_section) {
    size_t section_index;
    if ((ini_file == NULL) || (section == NULL) || (ini_section == NULL)) {
        return ini_invalid_parameters;
    }
    if (strlen(section) == 0) {
        return ini_invalid_parameters;
    }
    for (section_index = 0; section_index < ini_file->sections_size; section_index++) {
        if (strcmp(ini_file->sections[section_index].name, section) == 0) {
            *ini_section = &ini_file->sections[section_index];
            return ini_no_error;
        }
    }
    /* Didn't found the requested section */
    return ini_no_such_section;
}

enum Ini_File_Errors ini_file_find_property(struct Ini_File *const ini_file, const char *const section, const char *const key, char **value) {
    struct Ini_Section *ini_section;
    size_t property_index;
    enum Ini_File_Errors error;
    if ((ini_file == NULL) || (section == NULL) || (key == NULL) || (value == NULL)) {
        return ini_invalid_parameters;
    }
    if (strlen(key) == 0) {
        return ini_invalid_parameters;
    }
    error = ini_file_find_section(ini_file, section, &ini_section);
    if (error != ini_no_error) {
        return error;
    }
    for (property_index = 0; property_index < ini_section->properties_size; property_index++) {
        if (strcmp(ini_section->properties[property_index].key, key) == 0) {
            *value = ini_section->properties[property_index].value;
            return ini_no_error;
        }
    }
    /* Didn't found the requested property */
    return ini_no_such_property;
}

enum Ini_File_Errors ini_file_find_integer(struct Ini_File *const ini_file, const char *const section, const char *const key, long *integer) {
    char *value, *end;
    long i_value;
    enum Ini_File_Errors error;
    if (integer == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_file_find_property(ini_file, section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    i_value = strtol(value, &end, 10);
    if (*end != '\0') {
        return ini_not_integer;
    }
    *integer = i_value;
    return ini_no_error;
}

enum Ini_File_Errors ini_file_find_unsigned(struct Ini_File *const ini_file, const char *const section, const char *const key, unsigned long *uint) {
    char *value, *end;
    unsigned long ui_value;
    enum Ini_File_Errors error;
    if (uint == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_file_find_property(ini_file, section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    ui_value = strtoul(value, &end, 10);
    if (*end != '\0') {
        return ini_not_unsigned;
    }
    *uint = ui_value;
    return ini_no_error;
}

enum Ini_File_Errors ini_file_find_float(struct Ini_File *const ini_file, const char *const section, const char *const key, double *real) {
    char *value, *end;
    double d_value;
    enum Ini_File_Errors error;
    if (real == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_file_find_property(ini_file, section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    d_value = strtod(value, &end);
    if (*end != '\0') {
        return ini_not_float;
    }
    *real = d_value;
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
