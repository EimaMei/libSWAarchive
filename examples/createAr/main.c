#define SISWA_ARCHIVE_IMPLEMENTATION
#include "libSUarchive.h"


static char xmlData[] =
	"<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\" ?>\r\n"
	"<smth>\r\n\t"
		"<width>640</width>\r\n\t"
		"<height>480</height>\r\n"
	"</smth>";

static char msg[] = "Labas, pasauli!";


struct pos {
	uint64_t x, y;
};


int main(void) {
	/* Create the archive file */
	siArFile arFile = siswa_arCreateArContent(512);
	siswa_arEntryAdd(&arFile, "resolution.set.xml", xmlData, sizeof(xmlData));
	siswa_arEntryAdd(&arFile, "randomMessage.txt", msg, sizeof(msg));
	siswa_arEntryAdd(&arFile, "info.bin", &arFile.len, sizeof(size_t));

	{
		struct pos test;
		test.x = UINT64_MAX;
		test.y = INT64_MAX;
		siswa_arEntryUpdate(&arFile, "info.bin", &test, sizeof(test));
	}

	{ /* Write it into a file. */
		FILE* file = fopen("test.ar.00", "w");
		fwrite(arFile.data, arFile.len, 1, file);
		fclose(file);
	}

	{ /* List every entry in the archive's content and the info about it. */
		siArEntry* entry;
		while (siswa_arEntryPoll(&arFile, &entry)) {
			const char* filename = siswa_arEntryGetName(entry);

			printf(
				"%s:\n\t"
					"Size: %i\n\t"
					"Data size: %i\n\t"
					"Offset: %i\n",
				filename, entry->size, entry->dataSize, entry->offset
			);
		}
	}

	free(arFile.data); /* siswa_arFree() acts the same way. */
	return 0;
}


