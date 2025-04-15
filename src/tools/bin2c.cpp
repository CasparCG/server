#include <assert.h>
#include <cerrno>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s \"namespace\" \"constant_name\" \"input_filename\"\n", argv[0]);
        return -1;
    }
    char* fn = argv[3];
    FILE* f  = fopen(fn, "rb");

    if (f == nullptr) {
        fprintf(stderr, "Error opening file: %s\n", strerror(errno));
        return -1;
    }

    printf("namespace %s {\n", argv[1]);

    printf("const char %s[] = {\n", argv[2]);
    unsigned long n = 0;
    while (!feof(f)) {
        unsigned char c;
        if (fread(&c, 1, 1, f) == 0)
            break;
        if ('\r' == c) // ignore carret return
            continue;
        printf("0x%.2X,", (int)c);
        ++n;
        if (n % 10 == 0)
            printf("\n");
    }
    fclose(f);
    printf("0x00\n};\n");

    printf("}\n");

    return 0;
}
