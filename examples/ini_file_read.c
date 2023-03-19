/*------------------------------------------------------------------------------
 * SOURCE
 *------------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>

#include "../ini_file.h"

int error_callback(const char *const filename, const size_t line_number, const size_t column, const char *const line, const enum Ini_File_Errors error) {
    fprintf(stderr, "%s:%lu:%lu %s:\n%s\n", filename, line_number, column, ini_file_error_to_string(error), line);
    return 0;
}

/*------------------------------------------------------------------------------
 * MAIN
 *------------------------------------------------------------------------------
 */

int main(const int argc, const char **const argv) {
    struct Ini_File *ini_file;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s ini_file_name\n", argv[0]);
        return EXIT_FAILURE;
    }
    ini_file = ini_file_parse(argv[1], error_callback);
    if (ini_file == NULL) {
        fprintf(stderr, "Was not possible to parse the ini_file \"%s\"\n", argv[1]);
        return EXIT_FAILURE;
    }
    printf("\nThe properties retrieved from the the ini file \"%s\" are:\n\n", argv[1]);
    ini_file_print_to(ini_file, stdout);
    ini_file_free(ini_file);
    return EXIT_SUCCESS;
}

/*------------------------------------------------------------------------------
 * END
 *------------------------------------------------------------------------------
 */
