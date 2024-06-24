#define SISWA_ARCHIVE_IMPLEMENTATION
#include "libSUarchive.h"

#define countof(array) (sizeof(array) / sizeof(*array))


typedef struct {
	void* data;
	size_t len;
} fileInfo;


fileInfo readFile(const char* filename) {
	FILE* file;
	fileInfo res;

	SISWA_ASSERT_NOT_NULL(filename);

	file = fopen(filename, "rb");
	SISWA_ASSERT_NOT_NULL(file);

	fseek(file, 0, SEEK_END);
	res.len = ftell(file);
	rewind(file);

	res.data = malloc(res.len);
	fread(res.data, res.len, 1, file);
	fclose(file);

	return res;
}


static const char* filenames[] = {
	"examples/packAr/area22_enemyset.set.xml",
	"examples/packAr/area03_gimmickset.set.xml",
	"examples/packAr/system.set.xml",
	"examples/packAr/BaseEvil.set.xml",
};
static fileInfo files[countof(filenames)];


int main(void) {
	size_t totalSize = 0;
	siArFile ar;
	{ /* Read the contents of the files to pack. */
		size_t i;
		for (i = 0; i < countof(filenames); i += 1) {
			files[i] = readFile(filenames[i]);
			totalSize += files[i].len + strlen(filenames[i]) + sizeof(siArEntry) + 6;
		}
	}
	ar = siswa_arCreateContent(totalSize);

	{ /* Pack them all into one archive. */
		size_t i;
		for (i = 0; i < countof(files); i += 1) {
			siswa_arEntryAdd(&ar, filenames[i], files[i].data, files[i].len);
		}
	}

	{ /* Write it into a file. */
		FILE* file = fopen("pack.ar.00", "wb");
		fwrite(ar.data, ar.len, 1, file);
		fclose(file);
	}

	{ /* Free the files from memory. */
		size_t i;
		for (i = 0; i < countof(files); i += 1) {
			free(files[i].data);
		}
	}
	free(ar.data);

	return 0;
}
