#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t strlen_utf8(const char *str) {
    size_t len = 0;
    for (; *str; str++) {
        if ((*str & 0xc0) != 0x80) {
            len++;
        }
    }
    return len;
}

int main(void) {
    // Read a encoded string from a file
    FILE *fp = fopen("text.txt", "r,ccs=utf16le");
    if (fp == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    char str[4096];
    if (fgets(str, sizeof(str), fp) == NULL) {
        perror("Error in gets");
        //return EXIT_FAILURE;
    }
    printf("strlen=%ld, strlen_utf8=%ld, str=%s\n", strlen(str), strlen_utf8(str), str);
    return EXIT_SUCCESS;
}
#elif 1

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <locale.h>

int main() {
    // Set locale to support UTF-16LE encoding
    char *local = setlocale(LC_ALL, "en_US.UTF-16");
    if (local == NULL) {
        perror("setlocale");
        return EXIT_FAILURE;
    }
    printf("locale: %s\n", local);

    FILE* file = fopen("text.txt", "rb");
    if (file == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer for file contents
    wchar_t* file_contents = malloc(file_size);
    if (file_contents == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    printf("file_size=%d\n", file_size);
    // Read file contents
    size_t bytes_read = fread(file_contents, 1, file_size, file);
    printf("bytes_read=%ld\n", bytes_read);
    if (bytes_read != file_size) {
        printf("Error: Failed to read file\n");
        return 1;
    }

    // Close file
    fclose(file);

    // Convert wide string to multibyte string in UTF-8 encoding
    int buffer_size = wcstombs(NULL, file_contents, 0) + 1;
    printf("buffer_size=%d\n", buffer_size);
    char* buffer = malloc(buffer_size);

    // Print multibyte string to console
    printf("%s", buffer);

    // Free buffers
    free(file_contents);
    free(buffer);

    return 0;
}

#else

/* wcstombs example */
#include <stdio.h>      /* printf */
#include <stdlib.h>     /* wcstombs, wchar_t(C) */
#include <locale.h>

int main() {
  const wchar_t str[] = L"ç ~^ âãáà êẽéè õôóò";
  char buffer[32];
  int ret;

    setlocale(LC_ALL, "");

  printf ("wchar_t string: %ls \n",str);

  ret = wcstombs ( buffer, str, sizeof(buffer) );
  if (ret==32) buffer[31]='\0';
  if (ret) printf ("multibyte string: %s \n",buffer);

  return 0;
}

#endif