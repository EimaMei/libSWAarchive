#define SISWA_ARCHIVE_IMPLEMENTATION
#include "libSUarchive.h"


int main(void) {
	siArFile mergedAr;
	siByte buffer[512];
	siArFile arFiles[3];

	/* Open the archive files. */
	arFiles[0] = siswa_arMake("examples/mergeAr/test.ar.00");
	arFiles[1] = siswa_arMake("examples/mergeAr/gimmickSet.ar.00");
	arFiles[2] = siswa_arMake("examples/mergeAr/anotherGimmickSet.ar.00");

	/* Merge them all into one big one. anotherGimmickSet.ar.00's "area03_gimmickset.set.xml"
	 * entry gets ignored because that entry was already set by gimmickSet.ar.00. */
	mergedAr = siswa_arMergeMul(arFiles, sizeof(arFiles) / sizeof(*arFiles), buffer, 512);

	{ /* Write it into a file. */
		FILE* file = fopen("result.ar.00", "wb");
		fwrite(mergedAr.data, mergedAr.len, 1, file);
		fclose(file);
	}

	{ /* Generate .arl file and write it into a file */
		char tmp[128];
		siArlFile arl = siswa_arlCreateFromAr(mergedAr, tmp, 128);
		FILE* file = fopen("result.arl", "wb");
		fwrite(arl.data, arl.len, 1, file);
		fclose(file);
	}

	{ /* List every entry in the archive's content. */
		siArEntry* entry;
		while (siswa_arEntryPoll(&mergedAr, &entry)) {
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

	{ /* Free the dynamically allocated memory. We don't have to free buffer as
	it was allocated on the stack. */
		size_t i;
		for (i = 0; i < sizeof(arFiles) / sizeof(*arFiles); i++) {
			free(arFiles[i].data);
		}
	}
	return 0;
}
