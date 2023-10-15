#define SISWA_ARCHIVE_IMPLEMENTATION
#include "libSUarchive.h"


int main(void) {
	/* Prepare the variables/ */
	siArFile ar;
	uint64_t size;

	/* Open the .ar file and check for the compression type. */
	ar = siswa_arMake("res/BossPetra.ar.00");
	size = siswa_arGetDecompressedSize(ar);
	printf("Compression info: 'type' - %i, 'size' - %lu bytes\n", ar.compression, size);

	{ /* Decompress the content. */
		siByte* buffer = malloc(size);
		siswa_arDecompress(&ar, buffer, size, SISWA_TRUE);
	}

	{ /* Write the decompress data into a new file. */
		FILE* output = fopen("output.ar.00", "wb");
		fwrite(ar.data, ar.len, 1, output);
		fclose(output);
	}

	{ /* List every entry in the archive's content. */
		siArEntry* entry;
		while (siswa_arEntryPoll(&ar, &entry)) {
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

	free(ar.data);
	return 0;
}
