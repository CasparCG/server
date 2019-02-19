#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    assert(argc == 4);
    char* fn = argv[3];
    FILE* f  = fopen(fn, "rb");

    int count = 0;
    char* pch = strtok(argv[1], "::");
    while (pch != nullptr) {
        printf("namespace %s {\n", pch);
        pch = strtok(nullptr, "::");
        count++;
    }

    printf("const char %s[] = {\n", argv[2]);
    unsigned long n = 0;
    while (!feof(f)) {
        unsigned char c;
        if (fread(&c, 1, 1, f) == 0)
            break;
        printf("0x%.2X,", (int)c);
        ++n;
        if (n % 10 == 0)
            printf("\n");
    }
    fclose(f);
    printf("};\n");

    for (int i = 0; i < count; i++)
        printf("}\n");
}
