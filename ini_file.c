
/*------------------------------------------------------------------------------
 * SOURCE
 *------------------------------------------------------------------------------
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ini_file.h"

/* Most systems do not allow for a line greather than 4 kbytes */
#define MAX_LINE_SIZE 4096

#define INITIAL_NUMBER_OF_SECTIONS 10
#define INITIAL_NUMBER_OF_PROPERTIES 10

size_t get_file_size(FILE *const file) {
    long file_size;
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return (size_t)file_size;
}

/* Remember to free the memory allocated for the returned string */
char *get_content_from_file(const char *const file_name) {
	char *buffer = NULL;
	FILE *const file = fopen(file_name, "rb");
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
				free((void *)buffer);
				buffer = NULL;
			}
		}
		fclose(file);
	}
	return buffer;
}

struct Ini_File *ini_file_new(void) {
    struct Ini_File *ini_file = malloc(sizeof(struct Ini_File));
    if (ini_file != NULL) {
        ini_file->sections_size = 0;
        ini_file->sections_capacity = INITIAL_NUMBER_OF_SECTIONS;
        ini_file->sections = malloc(ini_file->sections_capacity * sizeof(struct Ini_Section));
        if (ini_file->sections == NULL) {
            free(ini_file);
            return NULL;
        }
        /* The first section is global to the file */
        if (ini_file_add_section(ini_file, "global") != 0) {
            ini_file_free(ini_file);
            return NULL;
        }
    }
    return ini_file;
}

void ini_file_free(struct Ini_File *const ini_file) {
    size_t i, j;
    if (ini_file == NULL) {
        return;
    }
    for (i = 0; i < ini_file->sections_size; i++) {
        free((void *)ini_file->sections[i].name);
        for (j = 0; j < ini_file->sections[i].properties_size; j++) {
            free((void *)ini_file->sections[i].properties[j].key);
            free((void *)ini_file->sections[i].properties[j].value);
        }
        free(ini_file->sections[i].properties);
        ini_file->sections[i].properties_capacity = 0;
        ini_file->sections[i].properties_size = 0;
    }
    free(ini_file->sections);
    ini_file->sections_capacity = 0;
    ini_file->sections_size = 0;
    free((void *)ini_file);
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
    size_t section_index;
    if (ini_file == NULL) {
        return;
    }
    for (section_index = 0; section_index < ini_file->sections_size; section_index++) {
        ini_section_print_to(&ini_file->sections[section_index], sink);
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
        "Expected a value, but found a comment",
        "Didn't found the requested section",
        "Didn't found the requested property",
        "The requested property is not a valid integer number",
        "The requested property is not a valid floating point number",
    };
#ifdef _Static_assert
    _Static_assert((NUMBER_OF_INI_FILE_ERRORS == (sizeof(error_messages)/sizeof(error_messages[0]))),
        "Don't forget to add the messages to this table");
#endif
    return error_messages[error];
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

/* TODO: We can have a large buffer of memory and store each new string in there */
static char *copy_sized_string(const char *const sized_str, const size_t len) {
    char *str = malloc(len + 1);
    if (str != NULL) {
        strncpy(str, sized_str, len);
        str[len] = '\0';
    }
    return str;
}

static int ini_file_parse_handle_error(Ini_File_Error_Callback callback, const char *const filename, const size_t line_number, const char *const line, const enum Ini_File_Errors error) {
    /* This function is called when we found an error in the parsing.
     * So, we report it to the user using the callback provided. 
     * If the callback returns an integer different from zero,
     * we end the parsing and return NULL. */
    if (callback != NULL) {
        return callback(filename, line_number, line, error);
    }
    return 0;
}

/* TODO: Check for repeated section and key names? */
/* TODO: Check for spaces inside of section and key names? */
/* TODO: Sort the sections and keys? This would allow us to use binary search */
/* TODO: we could allow keys and values to be strings delimited by "" or '',
 * which would allow us to use the characters "=,#,;" inside keys and values */

/* Remember to free the memory allocated for the returned ini file structure */
struct Ini_File *ini_file_parse(const char *const filename, Ini_File_Error_Callback callback) {
    enum Ini_File_Errors error;
    char line[MAX_LINE_SIZE];
    size_t line_number;
    FILE *file;
    struct Ini_File *ini_file = ini_file_new();
    if (ini_file == NULL) {
        ini_file_parse_handle_error(callback, filename, 0, NULL, ini_allocation);
        return NULL;
    }
    file = fopen(filename, "rb");
	if (file == NULL) {
        ini_file_parse_handle_error(callback, filename, 0, NULL, ini_couldnt_open_file);
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
            advance_string_until(&cursor, "]#;\n");
            if (*cursor != ']') {
                if (ini_file_parse_handle_error(callback, filename, line_number, line, ini_expected_clocing_bracket) != 0) {
                    goto ini_file_parse_error;
                }
                continue;
            }
            /* Compute length of the key string and remove trailing whitespaces */
            name_len = (size_t)(cursor - name);
            while ((name_len > 0) && (isspace((unsigned char)name[name_len - 1]))) {
                name_len--;
            }
            error = ini_file_add_section_sized(ini_file, name, name_len);
            if (error != ini_no_error) {
                if (ini_file_parse_handle_error(callback, filename, line_number, line, error) != 0) {
                    goto ini_file_parse_error;
                }
            }
            /* We just ignore the possible characters after the end of the declaration of the section */
            continue;
        }
        key = cursor;
        advance_string_until(&cursor, "=#;\n");
        if (*cursor != '=') {
            /* Found an error, so we report it to the user using the callback provided. 
             * In case of an error, if the callback returns an integer different from zero,
             * we end the parsing and return NULL. */
            if (ini_file_parse_handle_error(callback, filename, line_number, line, ini_expected_equals) != 0) {
                goto ini_file_parse_error;
            }
            continue;
        }
        /* Compute length of the key string and remove trailing whitespaces */
        key_len = (size_t)(cursor - key);
        while ((key_len > 0) && (isspace((unsigned char)key[key_len - 1]))) {
            key_len--;
        }
        cursor++;
        advance_white_spaces(&cursor);
        /* If it is a commment, we found an error */
        if (strchr("#;", *cursor) != NULL) {
            /* Found an error, so we report it to the user using the callback provided. 
             * In case of an error, if the callback returns an integer different from zero,
             * we end the parsing and return NULL. */
            if (ini_file_parse_handle_error(callback, filename, line_number, line, ini_expected_value_got_comment) != 0) {
                goto ini_file_parse_error;
            }
            continue;
        }
        value = cursor;
        advance_string_until(&cursor, "#;\n");
        /* Compute length of the value string and remove trailing whitespaces */
        value_len = (size_t)(cursor - value);
        while ((value_len > 0) && (isspace((unsigned char)value[value_len - 1]))) {
            value_len--;
        }
        error = ini_file_add_property_sized(ini_file, key, key_len, value, value_len);
        if (error != ini_no_error) {
            if (ini_file_parse_handle_error(callback, filename, line_number, line, error) != 0) {
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

enum Ini_File_Errors ini_file_add_section_sized(struct Ini_File *const ini_file, const char *const name, const size_t name_len) {
    struct Ini_Section *section;
    if ((ini_file == NULL) || (name == NULL)) {
        return ini_invalid_parameters;
    }
    if (name_len == 0) {
        return ini_invalid_parameters;
    }
    if ((ini_file->sections_size + 1) >= ini_file->sections_capacity) {
        const size_t new_cap = 2 * ini_file->sections_capacity;
        struct Ini_Section *const new_sections = realloc(ini_file->sections, new_cap * sizeof(struct Ini_Section));
        if (new_sections == NULL) {
            return ini_allocation;
        }
        ini_file->sections = new_sections;
        ini_file->sections_capacity = new_cap;
    }
    section = &ini_file->sections[ini_file->sections_size];
    section->name = copy_sized_string(name, name_len);
    if (section->name == NULL) {
        return ini_allocation;
    }
    section->properties_size = 0;
    section->properties_capacity = INITIAL_NUMBER_OF_PROPERTIES;
    section->properties = malloc(section->properties_capacity * sizeof(struct Key_Value_Pair));
    if (section->properties == NULL) {
        free(section->name);
        return ini_allocation;
    }
    ini_file->sections_size++;
    return ini_no_error;
}

enum Ini_File_Errors ini_file_add_section(struct Ini_File *const ini_file, const char *const name) {
    if (name == NULL) {
        return ini_invalid_parameters;
    }
    return ini_file_add_section_sized(ini_file, name, strlen(name));
}

enum Ini_File_Errors ini_file_add_property_sized(struct Ini_File *const ini_file, const char *const key, const size_t key_len, const char *const value, const size_t value_len) {
    struct Ini_Section *section;
    struct Key_Value_Pair *property;
    if ((ini_file == NULL) || (key == NULL) || (value == NULL)) {
        return ini_invalid_parameters;
    }
    if ((key_len == 0) || (value_len == 0)) {
        return ini_invalid_parameters;
    }
    if (ini_file->sections_size == 0) {
        return ini_allocation;
    }
    /* Insert the new property at the last section */
    section = &ini_file->sections[ini_file->sections_size - 1];
    if ((section->properties_size + 1) >= section->properties_capacity) {
        const size_t new_cap = 2 * section->properties_capacity;
        struct Key_Value_Pair *const new_properties = realloc(section->properties, new_cap * sizeof(struct Key_Value_Pair));
        if (new_properties == NULL) {
            return ini_allocation;
        }
        section->properties = new_properties;
        section->properties_capacity = new_cap;
    }
    property = &section->properties[section->properties_size];
    property->key = copy_sized_string(key, key_len);
    if (property->key == NULL) {
        return ini_allocation;
    }
    property->value = copy_sized_string(value, value_len);
    if (property->value == NULL) {
        free(property->key);
        return ini_allocation;
    }
    section->properties_size++;
    return ini_no_error;
}

enum Ini_File_Errors ini_file_add_property(struct Ini_File *const ini_file, const char *const key, const char *const value) {
    if ((key == NULL) || (value == NULL)) {
        return ini_invalid_parameters;
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
    enum Ini_File_Errors error;
    if (integer == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_file_find_property(ini_file, section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    *integer = strtol(value, &end, 10);
    if (*end != '\0') {
        return ini_not_integer;
    }
    return ini_no_error;
}

enum Ini_File_Errors ini_file_find_float(struct Ini_File *const ini_file, const char *const section, const char *const key, double *real) {
    char *value, *end;
    enum Ini_File_Errors error;
    if (real == NULL) {
        return ini_invalid_parameters;
    }
    error = ini_file_find_property(ini_file, section, key, &value);
    if (error != ini_no_error) {
        return error;
    }
    *real = strtod(value, &end);
    if (*end != '\0') {
        return ini_not_float;
    }
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
