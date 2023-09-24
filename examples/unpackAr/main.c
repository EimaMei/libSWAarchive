#define SISWA_ARCHIVE_IMPLEMENTATION
#include "libSUarchive.h"


int main(void) {
    siArFile ar = siswa_arMake("res/pan.ar.00");
    siArEntry* entry;

    while ((entry = siswa_arEntryPoll(&ar))) {
        char* filename = siswa_arEntryGetName(entry);
        void* data = siswa_arEntryGetData(entry);

        FILE* file = fopen(filename, "wb");
        fwrite(data, entry->dataSize, 1, file);
        fclose(file);
    }

    free(ar.data);
    return 0;
}