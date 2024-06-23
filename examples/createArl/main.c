#define SISWA_ARCHIVE_IMPLEMENTATION
#include "libSUarchive.h"


int main(void) {
	siArlFile arl = siswa_arlCreateContent(256, 1);
	siswa_arlEntryAdd(&arl, "Name", 0);
	siswa_arlEntryAdd(&arl, "some_file.xml", 0);
	siswa_arlEntryAdd(&arl, "old.txt", 0);

	{ /* Print every entry. */
		siArlEntry* entry;
		while (siswa_arlEntryPoll(&arl, &entry)) {
			char buf[256] = {0}; /* We have to copy it to some buffer as entry strings are not NULL-terminated. */
			strncpy(buf, entry->string, entry->len);
			printf("Entry: %s (%d bytes)\n", buf, entry->len);
		}
	}
	siswa_arlEntryRemove(&arl, "some_file.xml", 0);
	siswa_arlEntryUpdate(&arl, "old.txt", "new_file.txt", 0);

	{ /* Write it into a file. */
		FILE* file = fopen("test.arl", "w");
		fwrite(arl.data, arl.len, 1, file);
		fclose(file);
	}

	free(arl.data);
	return 0;
}

