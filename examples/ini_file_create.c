/*------------------------------------------------------------------------------
 * SOURCE
 *------------------------------------------------------------------------------
 */

#include "../ini_file.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRING_SIZE 1024

/* This function reads a string of size MAX_STRING_SIZE from stdin.
 * It returns 0 if no valid string was retrieved. */
 int get_string_from_stdin(const char *const prompt, char *string) {
    fputs(prompt, stdout);
    if (fgets(string, MAX_STRING_SIZE, stdin) == NULL) {
        return 0;
    }
    /* Replace space and new line characters by \0 */
    string[strcspn(string, " \t\r\n")] = '\0';
    /* Check if string is empty */
    return (string[0] != '\0');
 }

/*------------------------------------------------------------------------------
 * MAIN
 *------------------------------------------------------------------------------
 */

int main(void) {
    char filename[MAX_STRING_SIZE];
    char section[MAX_STRING_SIZE];
    char key[MAX_STRING_SIZE], value[MAX_STRING_SIZE];
    struct Ini_File *ini_file = ini_file_new();
    /* Instruction on how to use this application */
    printf("Following, type the requested fields of keys, values and section names.\n");
    printf("If you wish to create a new section, enter a empty key.\n");
    printf("If you wish to end the file, enter a empty section name.\n\n");
    while (1) {
        while (get_string_from_stdin("key:   ", key)) {
            if (!get_string_from_stdin("value: ", value)) {
                continue;
            }
            ini_file_add_property(ini_file, key, value);
        }
        if (!get_string_from_stdin("\nsection: ", section)) {
            break;
        }
        ini_file_add_section(ini_file, section);
    }
    if (get_string_from_stdin("\nPlease type the filename: ", filename)) {
        if (ini_file_save(ini_file, filename) == 0) {
            printf("The typed properties were saved to the file %s\n", filename);
        } else {
            fprintf(stderr, "It was not possible to save the typed properties to the file %s\n", filename);
            ini_file_free(ini_file);
            return EXIT_FAILURE;
        }
    } else {
        printf("The typed properties are:\n\n");
        ini_file_print_to(ini_file, stdout);
    }
    ini_file_free(ini_file);
    return EXIT_SUCCESS;
}

/*------------------------------------------------------------------------------
 * END
 *------------------------------------------------------------------------------
 */
