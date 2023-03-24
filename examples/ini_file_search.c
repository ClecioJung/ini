/*------------------------------------------------------------------------------
 * SOURCE
 *------------------------------------------------------------------------------
 */

#include "../ini_file.h"

#include <stdio.h>
#include <stdlib.h>

/*------------------------------------------------------------------------------
 * MAIN
 *------------------------------------------------------------------------------
 */

int main(const int argc, const char **const argv) {
    enum Ini_File_Errors error;
    struct Ini_File *ini_file;
    struct Ini_Section *ini_section;
    char *value;
    if ((argc < 2) || (argc > 4)) {
        fprintf(stderr, "Usage: %s ini_file_name [section] [key]\n", argv[0]);
        return EXIT_FAILURE;
    }
    ini_file = ini_file_parse(argv[1], NULL);
    if (ini_file == NULL) {
        fprintf(stderr, "It was not possible to parse the ini_file \"%s\"\n", argv[1]);
        return EXIT_FAILURE;
    }
    switch (argc) {
    case 2:
        ini_file_print_to(ini_file, stdout);
        break;
    case 3:
        /* First, try to find this name as a property in the global section */
        error = ini_file_find_property(ini_file, NULL, argv[2], &value);
        if (error == ini_no_error) {
            puts(value);
            break;
        }
        /* If it wasn't a property, try to fnd it as a section */
        error = ini_file_find_section(ini_file, argv[2], &ini_section);
        if (error != ini_no_error) {
            fprintf(stderr, "%s\n", ini_file_error_to_string(error));
            ini_file_free(ini_file);
            return EXIT_FAILURE;
        }
        ini_section_print_to(ini_section, stdout);
        break;
    case 4:
        error = ini_file_find_property(ini_file, argv[2], argv[3], &value);
        if (error != ini_no_error) {
            fprintf(stderr, "%s\n", ini_file_error_to_string(error));
            ini_file_free(ini_file);
            return EXIT_FAILURE;
        }
        puts(value);
        break;
    }
    ini_file_free(ini_file);
    return EXIT_SUCCESS;
}

/*------------------------------------------------------------------------------
 * END
 *------------------------------------------------------------------------------
 */
