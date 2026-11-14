#include "../3ds.h"

#include <cstring>

int main(int argc, char** argv)
{
    if (argc != 4)
        return 1;
    FILE* file = fopen(argv[1], "rb");
    if (!file)
        return 1;

    const char* names[] = {"Stem", "Egg"};
    bool ok = GoToChunk(CHUNK_MAIN, file) && GoToChunk(CHUNK_3DEDITOR, file);
    for (int i = 0; ok && i < 2; ++i) {
        const int size = GoToChunk(CHUNK_OBJECTBLOCK, file);
        const long start = ftell(file);
        char name[5];
        ok = size >= 11 && start >= 0 &&
             fread(name, sizeof(name), 1, file) == 1 &&
             std::memcmp(name, names[i], std::strlen(names[i]) + 1) == 0;
        if (ok && i == 1)
            ok = fseek(file, -1, SEEK_CUR) == 0; // "Egg" uses four bytes.
        ok = ok && ExportObject(argv[i + 2], file) &&
             fseek(file, start + size - 6, SEEK_SET) == 0;
    }
    fclose(file);
    if (!ok)
        fprintf(stderr, "ACT3: historical export of eggplant.3ds failed\n");
    return ok ? 0 : 1;
}
