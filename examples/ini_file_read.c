/*------------------------------------------------------------------------------
 * SOURCE
 *------------------------------------------------------------------------------
 */

#include "../ini_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
    clock_t start_time, end_time;
    double total_time;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s ini_file_name\n", argv[0]);
        return EXIT_FAILURE;
    }
    start_time = clock();
    ini_file = ini_file_parse(argv[1], error_callback);
    end_time = clock();
    if (ini_file == NULL) {
        fprintf(stderr, "Was not possible to parse the ini_file \"%s\"\n", argv[1]);
        return EXIT_FAILURE;
    }
    printf("\nThe properties retrieved from the the ini file \"%s\" are:\n\n", argv[1]);
    ini_file_print_to(ini_file, stdout);
    printf("\nCheck out this information of the INI file data structure:\n");
    ini_file_info(ini_file);
    total_time = 1000.0 * ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Time needed to parse the INI file: %f ms\n\n", total_time);
    ini_file_free(ini_file);
    return EXIT_SUCCESS;
}

/*------------------------------------------------------------------------------
 * END
 *------------------------------------------------------------------------------
 */
