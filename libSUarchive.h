/*
libSUarchive - a light, fast and portable library for handling Sonic Unleashed's archive file formats (.ar/.arl).
1. Introduction
===========================================================================
	- To use the library, you must do the following in EXACTLY _one_ C/C++ file:
	```c
		#define SISWA_ARCHIVE_IMPLEMENTATION
		#include "libSUarchive.h"
    ```
    Once that's set, other files do not require the '#define' line.

    - There are two primary methods of using the libSUarchive library. The first
    one is to simply use 'siswa_<ar/arl>Make(const char*)`, which uses the C File
    IO functions to read the data from the given file path, as well as allocates
    the content into the heap. This function requires <stdlib.h>, as well as to call
    'siswa_<ar/arl>Free`/`free(var.data)` at the end to free the data from RAM.

    The second method is using `siswa_<arl/arl>MakeBuffer(void*, size_t)', which
    asks for both the buffer and length of said buffer. This method doesn't use any
    stdlib/stdio.h functions or allocates anything to the memory, as it's now the
    user's liberty to read and input the archive file data.

    Both methods also contain expanded versions (siswa_<funct>Ex), where you can
    either specify for more memory to be allocated (for something like adding more
    entries) or to specify the full capacity of the buffer.

2. Configuration macros
===========================================================================
    - For any of the macros to work, you must _always_ define it before including
    the library. Example:
	```c
		#define SISWA_ARCHIVE_IMPLEMENTATION
        #define SISWA_NO_STDLIB
		#include "libSUarchive.h"
    ```

    1. SISWA_NO_STDLIB:
        - This macro makes siswa not include <stdlib.h>, as well as disables
        functions that use it. This includes functions like 'siswa_arMake', as it
        both uses 'malloc' and 'fopen', meaning you must use 'siswa_arMakeBuffer'
        instead.
    2. SISWA_NO_ASSERT:
        - Disables any assertions in the codebase. Recommended to use for when
        building the release version of the program. Defining NDEBUG achieves
        the same result.
    3. SISWA_NO_STDINT
        - Disables the includes of <stdint.h> and <stddef.h>. However, the library
        expects for certain int types to exist already with their correct sizes
        before the include of the library, otherwise the app won't compile.
        The list of types being:
            - uint8_t           - 1 byte, unsigned.
            - uint16_t, int16_t - 2 bytes, both signed and unsigned.
            - uint32_t, int32_t - 4 bytes, both signed and unsigned.
            - uint64_t, int64_t - 8 bytes, both signed and unsigned.
            - size_t            - must equal to 'sizeof(void*)' bytes, unsigned.
    4. SISWA_NO_DECOMPRESSION
        - Completely disables any decompression features in the library.
    5. SISWA_DEFINE_CUSTOM_DEFLATE_DECOMPRESSION
        - Disables siswa's implementation of Deflate decompression, but keeps
        intact SEGS decompression functions like 'siswa_arDecompressSegs'.
        In turn the user must define the function 'siswa_decompressDeflate'
        with the correct output.
    6. SISWA_USE_PRAGMA_PACK
        - Uses '#pragma pack(push, 1)' for every struct inside the file to achieve
        the required struct sizes. Turned off by default for portability reasons.

3. Other
===========================================================================
CREDITS:
    - HedgeServer (discord) - helping out to figure out the behaviour and format
        of .ar/.arl files.
    - HedgeLib (https://github.com/Radfordhound/HedgeLib) - some of the header
        documentation taken from 'hl_hh_archive.h'.
    - sinfl.h (https://github.com/vurtun/lib) - the base code forming the Deflate
        decompressing function in siswa.
    - General Schnitzel - some general help, moral support(?) as well as providing
        useful .ar files to test the library with.
LICENSE:
	- This software is licensed under the zlib license (see the LICENSE in the
    bottom of the file).

NOTICE:
	- This library is _slightly_ experimental and features may not work as
	expected.
	- This also means that functions may not be documented fully.
*/

#ifndef SISWA_INCLUDE_SISWA_H
#define SISWA_INCLUDE_SISWA_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef SISWA_NO_STDINT
    #include <stdint.h>
    #include <stddef.h>
#endif


#ifndef SISWA_NO_STDLIB
    #include <stdlib.h>
    #include <stdio.h>
#endif
#include <string.h> /* memcpy and stuff */

#ifndef SISWA_NO_ASSERT
    #include <assert.h> /* assert */
#endif



#if !defined(SISWA_ASSERT) && !defined(SISWA_NO_ASSERT)
    #define SISWA_ASSERT(x, msg) assert(x); (void)(msg)

#elif !defined(SISWA_ASSERT) && defined(SISWA_NO_ASSERT)
    #define SISWA_ASSERT(x, msg)  ((void)(x), (void)(msg))
#endif

#define SISWA_ASSERT_NOT_NULL(ptr) SISWA_ASSERT((ptr) != NULL, #ptr " must not be NULL.")

#define SISWA_STATIC_ASSERT2(cond, msg)  typedef char static_assertion_##msg[(!!(cond)) * 2 - 1] /* NOTE(EimaMei): This is absolutely stupid but somehow it works so who cares */
#define SISWA_STATIC_ASSERT1(cond, line) SISWA_STATIC_ASSERT2(cond, line)
#define SISWA_STATIC_ASSERT(cond)        SISWA_STATIC_ASSERT1(cond, __LINE__)

SISWA_STATIC_ASSERT(sizeof(uint8_t) == 1);
SISWA_STATIC_ASSERT(sizeof(uint16_t) == 2);
SISWA_STATIC_ASSERT(sizeof(int32_t) == 4);
SISWA_STATIC_ASSERT(sizeof(uint32_t) == 4);
SISWA_STATIC_ASSERT(sizeof(int64_t) == 8);
SISWA_STATIC_ASSERT(sizeof(uint64_t) == 8);
SISWA_STATIC_ASSERT(sizeof(size_t) == sizeof(void*));

#ifndef siByte
    typedef uint8_t siByte;
#endif
typedef uint32_t siBool;


/* The alignment value set in the archive files. The actual proper definition would be
 * the processor's computing bit (hence why Unleashed uses 64 for the alignment as
 * the X360/PS3 are both 64-bit). If for whatever reason you're planning to use the
 * library on different computing bit CPUs, you should change this value to the
 * value that matches the computing bit of the CPU, or set it as '(sizeof(size_t) * 8)'.*/
#define SISWA_DEFAULT_HEADER_ALIGNMENT 64

#define SISWA_DEFAULT_STACK_SIZE 8 * 1024 /* NOTE(EimaMei): Used for storing string pointers.
                                          The higher the value, the less likely a hash collision will happen.
                                          8kb is a good middleground balance on the major OSses, but probably not
                                          for actual embedded systems.*/

#define SISWA_SUCCESS 1
#define SISWA_FAILURE 0
#define SISWA_TRUE    1
#define SISWA_FALSE   0

/* Length of the array can be anything from 1 to beyond. */
#define SISWA_UNSPECIFIED_LEN 1

#define SISWA_IDENTIFIER_ARL2 0x324C5241
#define SISWA_IDENTIFIER_XCOMPRESSION 0xEE12F50F
#define SISWA_IDENTIFIER_SEGS 0x73676573

#ifdef SISWA_USE_PRAGMA_PACK
    #pragma pack(push, 1)
#endif

typedef enum {
    SISWA_COMPRESSION_NONE,
    SISWA_COMPRESSION_X,
    SISWA_COMPRESSION_SEGS
} siCompressType;

typedef struct {
    uint32_t identifier;
    uint16_t dummy;
    uint16_t chunks;
    uint32_t fullSize;
    uint32_t fullZsize;
} siSegsHeader;
SISWA_STATIC_ASSERT(sizeof(siSegsHeader) == 16);

typedef struct {
    uint16_t zSize;
    uint16_t size;
    uint32_t offset;
} siSegsEntry;
SISWA_STATIC_ASSERT(sizeof(siSegsEntry) == 8);

typedef struct { /* NOTE(EimaMei): All credit goes to https://github.com/mistydemeo/quickbms/blob/master/unz.c */
    uint32_t identifier;
    uint16_t version;
    uint16_t reserved;
    uint32_t contextFlags;
    uint32_t flags;
    uint32_t windowSize;
    uint32_t compressionPartitionSize;
    uint64_t uncompressedSize;
    uint64_t compressedSize;
    uint32_t uncompressedBlockSize;
    uint32_t compressedBlockSizeMax;
} siXCompHeader;
SISWA_STATIC_ASSERT(sizeof(siXCompHeader) == 48);


typedef struct {
    /* Always 0. */
    uint32_t unknown;
    /* Size of the header struct. Same as sizeof(siArHeader), always 16 bytes. */
    uint32_t headerSizeof;
    /* Size of the entry struct. Same as sizeof(siArEntry), always 20 bytes. */
    uint32_t entrySizeof;
    /* Alignment that's used, usually 64 for 64-bit CPUs. */
    uint32_t alignment;
} siArHeader;
SISWA_STATIC_ASSERT(sizeof(siArHeader) == 16);


typedef struct {
    /* Entire size of the entry. Equivalent to '.dataSize + .offset + sizeof(siArEntry) = .size'. */
    uint32_t size;
    /* Size of the file data itself. */
    uint32_t dataSize;
    /* Pointer offset to the data. Usually equivalent to 'strlen(name) + pad + sizeof(siArEntry) = .offset'. */
    uint32_t offset;
    /* An unsigned 64-bit file date int. Can be expressed as a pointer to an uint64_t. */
    uint8_t filedate[8];
} siArEntry;
SISWA_STATIC_ASSERT(sizeof(siArEntry) == 20);


typedef struct {
    /* Pointer to the contents of the data. If `siswa_xMake' or 'siswa_xCreateArContent'
     * was used, the data is malloced and must be freed. */
    siByte* data;
    /* Current length of the content. Changes between entry modifications. */
    size_t len;
    /* Total memory capacity of '.data'. */
    size_t cap;

    /* Compress type of the content. */
    siCompressType compression;
    /* Current entry offset, modified by 'siswa_xEntryPoll'. Should not be modified otherwise.*/
    size_t curOffset;
} siArFile;


/* Creates a 'siArFile' structure from an '.ar' file. */
siArFile siswa_arMake(const char* path);
/* Creates a 'siArFile' structure from an '.ar' file and adds additional space to the capacity. */
siArFile siswa_arMakeEx(const char* path, size_t additionalAllocSpace);
/* Creates a 'siArFile' structure from an archive file's content in memory. */
siArFile siswa_arMakeBuffer(void* data, size_t len);
/* Creates a 'siArFile' structure from an archive file's content in memory, while also setting the capacity. */
siArFile siswa_arMakeBufferEx(void* data, size_t len, size_t capacity);

/* Creates a 'siArFile' structure and allocates an archive file in memory from the provided capacity. */
siArFile siswa_arCreateArContent(size_t capacity);
/* Creates a 'siArFile' structure and creates an archive file in memory from the provided buffer and capacity. */
siArFile siswa_arCreateArContentEx(void* buffer, size_t capacity);

/* Gets the header of the archive file. */
siArHeader* siswa_arGetHeader(siArFile arFile);
/* Gets the total entry count of the archive file. */
size_t siswa_arGetEntryCount(siArFile arFile);

/* Polls for the next entry in the archive. It reaches the end once it hits NULL. */
siArEntry* siswa_arEntryPoll(siArFile* arFile);
/* Resets the entry offset back to the start. */
void siswa_arOffsetReset(siArFile* arFile);
/* Finds an entry matching the provided name. Returns NULL if the entry doesn't exist. */
siArEntry* siswa_arEntryFind(siArFile arFile, const char* name);
/* Finds an entry matching the provided name with length. Returns NULL if the entry doesn't exist. */
siArEntry* siswa_arEntryFindEx(siArFile arFile, const char* name, size_t nameLen);

/* Gets the name of the provided entry. */
char* siswa_arEntryGetName(siArEntry* entry);
/* Gets the data of the provided entry. */
void* siswa_arEntryGetData(siArEntry* entry);

/* Adds a new entry in the archive. Fails if the entry name already exists or the capacity is too low. */
siBool siswa_arEntryAdd(siArFile* arFile, const char* name, void* data,
        uint32_t dataSize);
/* Adds a new entry in the archive. Fails if the entry name already exists or the capacity is too low. */
siBool siswa_arEntryAddEx(siArFile* arFile, const char* name, size_t nameLen,
        void* data, uint32_t dataSize);
/* Removes an entry in the archive. Fails if the entry doesn't exist. */
siBool siswa_arEntryRemove(siArFile* arFile, const char* name);
/* Removes an entry in the archive. Fails if the entry doesn't exist. */
siBool siswa_arEntryRemoveEx(siArFile* arFile, const char* name, size_t nameLen);
/* Updates the entry inside the archive. Fails if the entry doesn't exist. */
siBool siswa_arEntryUpdate(siArFile* arFile, const char* name, void* data,
        uint32_t dataSize);
/* Updates the entry inside the archive. Fails if the entry doesn't exist. */
siBool siswa_arEntryUpdateEx(siArFile* arFile, const char* name, size_t nameLen,
        void* data, uint32_t dataSize);

/* Creates a new archive by merging two archives into it, with the content being
 * written to 'outBuffer'. Any duplicating entries from the archives get ignored.
 * Fails if the capacity is too low to fit the two archive files in 'outBuffer'. */
siArFile siswa_arMerge(siArFile ars[2], void* outBuffer, size_t capacity);
/* Creates a new archive by merging all of the archive files into it, with the content
 * being written to 'outBuffer'. Any duplicating entries from the archives get ignored.
 * Fails if the capacity is too low to fit all of the archive files in 'outBuffer'. */
siArFile siswa_arMergeMul(siArFile* arrayOfArs, size_t arrayLen, void* outBuffer,
        size_t capacity);

#ifndef SISWA_NO_DECOMPRESSION
/* Decompresses the given archive file depending on the contents of the data and
 * writes the decompressed data into 'out'. This also sets 'arl.data' to 'out'.
 * Setting 'freeCompData' to true will do 'free(arl.data)', freeing the compressed
 * data from memory. */
void siswa_arDecompress(siArFile* arl, siByte* out, size_t capacity, siBool freeCompData);
/* Decompresses the given archive linker file using SEGS decompression and writes
 * the decompressed data into 'out'. This also sets 'arl.data' to 'out'. Setting
 * 'freeCompData' to true will do 'free(arl.data)', freeing the compressed
 * data from memory. */
void siswa_arDecompressSegs(siArFile* arl, siByte *out, size_t capacity, siBool freeCompData);
/* Gets the exact, raw decompressed size of the data if it's X or SEGS compressed. */
uint64_t siswa_arGetDecompressedSize(siArFile ar);
#endif

/* Frees arFile.buffer. Same as doing free(arFile.data) */
void siswa_arFree(siArFile arFile);

typedef struct {
    /* 'ARL2' at the start of the file. */
    uint32_t identifier;
    /* Total number of archive files. */
    uint32_t archiveCount;
    /* The amount of bytes for each archive. Length of the array is 'archiveCount'.
       archiveSizes[0] gives the 1st archive file's len, archiveSizes[1] gives
       the 2nd and so forth. */
    uint32_t archiveSizes[SISWA_UNSPECIFIED_LEN];
} siArlHeader;
SISWA_STATIC_ASSERT(sizeof(siArlHeader) == 12);

typedef siArFile siArlFile;

typedef struct {
    /* Length of the filename. */
    uint8_t len;
    /* The filename The string is NOT null-terminated! */
    char string[SISWA_UNSPECIFIED_LEN];
} siArlEntry;
SISWA_STATIC_ASSERT(sizeof(siArlEntry) == 2);


/* Creates a 'siArlFile' structure from an '.arl' file. */
siArlFile siswa_arlMake(const char* path);
/* Creates a 'siArlFile' structure from an '.arl' file and adds additional space
 * to the capacity. */
siArlFile siswa_arlMakeEx(const char* path, size_t additionalAllocSpace);
/* Creates a 'siArlFile' structure from an archive linker's content in memory. */
siArlFile siswa_arlMakeBuffer(void* data, size_t len);
/* Creates a 'siArlFile' structure from an archive linker's content in memory,
 * while also setting the capacity. */
siArlFile siswa_arlMakeBufferEx(void* data, size_t len, size_t capacity);

/* Creates a 'siArlFile' structure and allocates an archive linker file in memory
 * from the provided capacity. */
siArlFile siswa_arlCreateArlContent(size_t capacity, size_t archiveCount);
/* Creates a 'siArlFile' structure and creates an archive linker in memory from the
 * provided buffer and capacity. */
siArlFile siswa_arlCreateArlContentEx(void* buffer, size_t capacity, size_t archiveCount);

/* Generates an archive linker in 'outBuffer' from the provided archive. */
siArlFile siswa_arlCreateFromAr(siArFile arFile, void* outBuffer, size_t capacity);
/* Generates an archive linker in 'outBuffer' from the provided multiple archives. */
siArlFile siswa_arlCreateFromArMul(siArFile* arrayOfArs, size_t arrayLen,
        void* outBuffer, size_t capacity);

/* Gets the header of the archive linker. */
siArlHeader* siswa_arlGetHeader(siArlFile arlFile);
/* Gets the total entry count of the archive linker. */
size_t siswa_arlGetEntryCount(siArlFile arlFile);
/* Gets the actual length of the linker's header. */
size_t siswa_arlGetHeaderLength(siArlFile arlFile);

/* Polls for the next entry in the archive linker. It reaches the end once it hits
 * NULL. */
siArlEntry* siswa_arlEntryPoll(siArlFile* arlfile);
/* Resets the entry offset back to the start. */
void siswa_arlOffsetReset(siArFile* arFile);
/* Finds an entry matching the provided name. Returns NULL if the entry doesn't
 * exist. */
siArlEntry* siswa_arlEntryFind(siArlFile arlfile, const char* name);
/* Finds an entry matching the provided name with length. Returns NULL if the entry
 * doesn't exist. */
siArlEntry* siswa_arlEntryFindEx(siArlFile arlfile, const char* name, size_t nameLen);

/* Adds a new entry in the archive linker. Fails if the entry name already exists
 * or the capacity is too low. */
siBool siswa_arlEntryAdd(siArlFile* arlFile, const char* name, size_t archiveIndex);
/* Adds a new entry in the archive linker. Fails if the entry name already exists
 * or the capacity is too low. */
siBool siswa_arlEntryAddEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
        size_t archiveIndex);
/* Removes an entry in the archive linker. Fails if the entry doesn't exist. */
siBool siswa_arlEntryRemove(siArlFile* arlFile, const char* name, size_t archiveIndex);
/* Removes an entry in the archive linker. Fails if the entry doesn't exist. */
siBool siswa_arlEntryRemoveEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
        size_t archiveIndex);
/* Updates the entry inside the archive linker. Fails if the entry doesn't exist. */
siBool siswa_arlEntryUpdate(siArlFile* arlFile, const char* name, const char* newName,
        size_t archiveIndex);
/* Updates the entry inside the archive linker. Fails if the entry doesn't exist. */
siBool siswa_arlEntryUpdateEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
        const char* newName, uint8_t newNameLen, size_t archiveIndex);

#if 0 /* TODO */
/* Creates a new archive linker by merging two linkers into it, with the content
 * being written to 'outBuffer'. Any duplicating entries from the archive linkers
 * get ignored. Fails if the capacity is too low to fit the two archive files in
 * 'outBuffer'. */
siArlFile siswa_arlMerge(siArlFile arls[2], void* outBuffer, size_t capacity);
/* Creates a new archive linker by merging all of the linkers files into it, with
 * the content being written to 'outBuffer'. Any duplicating entries from the
 * archive linkers get ignored Fails if the capacity is too low to fit all of
 * the archive linkers in 'outBuffer'. */
siArlFile siswa_arlMergeMul(siArlFile* arrayOfArls, size_t arrayLen, void* outBuffer,
        size_t capacity);
#endif

#ifndef SISWA_NO_DECOMPRESSION
/* Decompresses the given archive linker file depending on the contents of the data
 * and writes the decompressed data into 'out'. This also sets 'arl.data' to 'out'.
 * Setting 'freeCompData' to true will do 'free(arl.data)', freeing the compressed
 * data from memory. */
void siswa_arlDecompress(siArlFile* arl, siByte* out, size_t capacity, siBool freeCompData);
/* Decompresses the given archive linker file using SEGS (Deflate) decompression
 * and writes the decompressed data into 'out'. This also sets 'arl.data' to 'out'.
 * Setting 'freeCompData' to true will do 'free(arl.data)', freeing the compressed
 * data from memory. */
void siswa_arlDecompressSegs(siArlFile* arl, siByte *out, size_t capacity, siBool freeCompData);
/*  Decompresses the given archive linker file using XCompression (LZX) decompression
 * and writes the decompressed data into 'out'. This also sets 'arl.data' to 'out'.
 * Setting 'freeCompData' to true will do 'free(arl.data)', freeing the compressed
 * data from memory. */
void siswa_arlDecompressXComp(siArlFile* arl, siByte* out, size_t capacity, siBool freeCompData);
/* Gets the exact, raw decompressed size of the data if it's X or SEGS compressed. */
uint64_t siswa_arlGetDecompressedSize(siArFile ar);
#endif

/* Frees arlFile.buffer. Same as doing free(arlFile.data) */
void siswa_arlFree(siArlFile arlFile);



#ifndef SISWA_NO_DECOMPRESSION
/* Decompresses the given buffer using Deflate decompression and writes it into
 * 'out'. Returns the length of the decompressed data. */
size_t siswa_decompressDeflate(siByte* data, size_t length, siByte* out, size_t capacity);
/* (TODO) Decompresses the given buffer using XCompression (LZX) decompression and
 * writes it into 'out'. Returns the length of the decompressed data. */
/* siswa_decompressXComp(siByte* data, size_t length, siByte* out, size_t capacity) */
#endif


#if defined(SISWA_ARCHIVE_IMPLEMENTATION)

/* autism macros */
#define siswa_swap16(x) \
    ((uint16_t)((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))
#define siswa_swap32(x)					\
    ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >> 8)	\
    | (((x) & 0x0000ff00u) << 8) | (((x) & 0x000000ffu) << 24))
#define siswa_swap64(x)			\
  ((((x) & (uint64_t)0xFF00000000000000) >> 56)	\
   | (((x) & (uint64_t)0x00FF000000000000) >> 40)	\
   | (((x) & (uint64_t)0x0000FF0000000000) >> 24)	\
   | (((x) & (uint64_t)0x000000FF00000000) >> 8)	\
   | (((x) & (uint64_t)0x00000000FF000000) << 8)	\
   | (((x) & (uint64_t)0x0000000000FF0000) << 24)	\
   | (((x) & (uint64_t)0x000000000000FF00) << 40)	\
   | (((x) & (uint64_t)0x00000000000000FF) << 56))


#if 1 /* NOTE(EimaMei): Internal hashing and function stuff, can ignore */

static
siBool siswa_isLittleEndian(void) {
    int32_t val = 1;
    return (int32_t)*((uint8_t*)&val) == 1;
}

typedef struct {
    char** entries;
    size_t capacity;
} siHashTable;

#define SI_FNV_OFFSET 14695981039346656037UL
#define SI_FNV_PRIME 1099511628211UL

static
uint64_t si_hash_key(const char* key) {
    uint64_t hash = SI_FNV_OFFSET;
    const char* p;
    for (p = key; *p; p++) {
        hash ^= (uint64_t)(*p);
        hash *= SI_FNV_PRIME;
    }
    return hash;
}

siHashTable* si_hashtable_make_reserve(void* mem, size_t capacity) {
    siHashTable* table = (siHashTable*)mem;
    table->capacity = capacity;
    table->entries = (char**)(table + 1);
    memset(table->entries, 0, capacity * sizeof(char*)); /* TODO(EimaMei): Do we need this? */

    return table;
}

uint32_t si_hashtable_exists(siHashTable* ht, const char* key) {
    uint64_t hash = si_hash_key(key);
    size_t index = (size_t)(hash & (uint64_t)(ht->capacity - 1));

    char** entry = ht->entries + index;
    char** old_entry = ht->entries + index;
    char** end = ht->entries + ht->capacity;
    do {
        if (*entry == NULL) {
            goto increment_entry;
        }

        if (*key == **entry && strcmp(key + 1, *entry + 1) == 0) {
            return SISWA_SUCCESS;
        }

increment_entry:
        entry += 1;
        if (entry == end) {
            entry = ht->entries;
        }
    } while (entry != old_entry);

    return SISWA_FAILURE;
}

char** si_hashtable_set(siHashTable* ht, const char* allocatedStr) {
    uint64_t hash = si_hash_key(allocatedStr);
    size_t index = (size_t)(hash & (uint64_t)(ht->capacity - 1));

    char** entry = ht->entries + index;
    char** end = ht->entries + ht->capacity;

    while (*entry != NULL) {
        entry += 1;
        if (entry == end) {
            entry = ht->entries;
        }
    }
    *entry = (char*)allocatedStr;

    return ht->entries + index;
}
#endif
siArFile siswa_arMake(const char* path) {
    return siswa_arMakeEx(path, 0);
}
#ifndef SISWA_NO_STDLIB
siArFile siswa_arMakeEx(const char* path, size_t additionalAllocSpace) {
    FILE* file;
    siByte* data;
    size_t dataLen;
    siArFile ar;

    SISWA_ASSERT_NOT_NULL(path);

    file = fopen(path, "rb");
    SISWA_ASSERT_NOT_NULL(file);

    fseek(file, 0, SEEK_END);
    dataLen = ftell(file);
	rewind(file);

    data = (siByte*)malloc(dataLen + additionalAllocSpace);
    fread(data, dataLen, 1, file);

    ar = siswa_arMakeBufferEx(data, dataLen, dataLen + additionalAllocSpace);
    fclose(file);

    return ar;
}
#endif
siArFile siswa_arMakeBuffer(void* data, size_t len) {
    return siswa_arMakeBufferEx(data, len, len);
}
siArFile siswa_arMakeBufferEx(void* data, size_t len, size_t capacity) {
    siArFile ar;
    uint32_t identifier;

    SISWA_ASSERT_NOT_NULL(data);
    SISWA_ASSERT(len <= capacity, "The length cannot be larger than the total capacity");

    identifier = *(uint32_t*)data;

    if (identifier == SISWA_IDENTIFIER_XCOMPRESSION) {
        ar.compression = SISWA_COMPRESSION_X;
    }
    else if (identifier == SISWA_IDENTIFIER_SEGS) {
        ar.compression = SISWA_COMPRESSION_SEGS;
    }
    else {
        SISWA_ASSERT(identifier != SISWA_IDENTIFIER_ARL2, "Use 'siswa_arlFileOpen' for ARL2 files!");
        ar.compression = SISWA_COMPRESSION_NONE;
    }
    /* SISWA_ASSERT(identifier != SISWA_IDENTIFIER_XCOMPRESSION, "Support for XCompressed files is not available."); */

    ar.data = (siByte*)data;
    ar.len = len;
    ar.cap = capacity;
    ar.curOffset = sizeof(siArHeader);

    return ar;
}

#ifndef SISWA_NO_STDLIB
siArFile siswa_arCreateArContent(size_t capacity) {
    return siswa_arCreateArContentEx(malloc(capacity + sizeof(siArHeader)), capacity);
}
#endif
siArFile siswa_arCreateArContentEx(void* buffer, size_t capacity) {
    siArHeader header;
    siArFile ar;

    SISWA_ASSERT_NOT_NULL(buffer);
    SISWA_ASSERT(capacity >= sizeof(siArHeader), "Capacity must be at least equal to or be higher than 'sizeof(siArHeader)'.");

    header.unknown = 0;
    header.headerSizeof = sizeof(siArHeader);
    header.entrySizeof = sizeof(siArEntry);
    header.alignment = SISWA_DEFAULT_HEADER_ALIGNMENT;
    memcpy(buffer, &header, sizeof(siArHeader));

    ar.data = (siByte*)buffer;
    ar.len = sizeof(siArHeader);
    ar.cap = capacity;
    ar.compression = SISWA_COMPRESSION_NONE;
    ar.curOffset = sizeof(siArHeader);

    return ar;
}

siArHeader* siswa_arGetHeader(siArFile arFile) {
    return (siArHeader*)arFile.data;
}
size_t siswa_arGetEntryCount(siArFile arFile) {
    size_t count = 0;
    while (siswa_arEntryPoll(&arFile) != NULL) {
        count += 1;
    }

    return count;
}

siArEntry* siswa_arEntryPoll(siArFile* arFile) {
    siArEntry* entry = (siArEntry*)(arFile->data + arFile->curOffset);
    if (arFile->curOffset >= arFile->len) {
        siswa_arOffsetReset(arFile);
        return NULL;
    }

    arFile->curOffset += entry->size;
    return entry;
}
void siswa_arOffsetReset(siArFile* arFile) {
    SISWA_ASSERT_NOT_NULL(arFile);
    arFile->curOffset = sizeof(siArHeader);
}
siArEntry* siswa_arEntryFind(siArFile arFile, const char* name) {
    return siswa_arEntryFindEx(arFile, name, strlen(name));
}
siArEntry* siswa_arEntryFindEx(siArFile arFile, const char* name, size_t nameLen) {
    siArEntry* entry;
    arFile.curOffset = sizeof(siArHeader);

    SISWA_ASSERT_NOT_NULL(name);

    while ((entry = siswa_arEntryPoll(&arFile)) != NULL) {
        char* entryName = siswa_arEntryGetName(entry);
        if (*name == *entryName && strncmp(name + 1, entryName + 1, nameLen - 1) == 0) {
            return entry;
        }
    }

    return NULL;
}

char* siswa_arEntryGetName(siArEntry* entry) {
    return (char*)entry + sizeof(siArEntry);
}
void* siswa_arEntryGetData(siArEntry* entry) {
    return (siByte*)entry + entry->offset;
}

siBool siswa_arEntryAdd(siArFile* arFile, const char* name, void* data, uint32_t dataSize) {
    return siswa_arEntryAddEx(arFile, name, strlen(name), data, dataSize);
}
siBool siswa_arEntryAddEx(siArFile* arFile, const char* name, size_t nameLen, void* data, uint32_t dataSize) {
    siArEntry newEntry;
    siByte* dataPtr;
    size_t offset = sizeof(siArHeader);

    SISWA_ASSERT_NOT_NULL(name);
    SISWA_ASSERT_NOT_NULL(data);

    {
        siArEntry* entry;
        siArFile tmpArFile = *arFile;

        while ((entry = siswa_arEntryPoll(&tmpArFile)) != NULL) {
            const char* entryName = siswa_arEntryGetName(entry);
            offset = tmpArFile.curOffset;

            if (*name == *entryName && strncmp(name + 1, entryName + 1, nameLen - 1) == 0) {
                return SISWA_FAILURE;
            }
        }
    }
    newEntry.size = dataSize + nameLen + 1 + sizeof(siArEntry);
    newEntry.dataSize = dataSize;
    newEntry.offset = nameLen + 1 + sizeof(siArEntry);
    memset(newEntry.filedate, 0, sizeof(uint64_t));

    SISWA_ASSERT(offset + newEntry.size < arFile->cap, "Not enough space inside the buffer to add a new entry.");
    dataPtr = arFile->data + offset;

    memcpy(dataPtr, &newEntry, sizeof(siArEntry));
    dataPtr += sizeof(siArEntry);

    memcpy(dataPtr, name, nameLen);
    dataPtr += nameLen;
    *dataPtr = '\0';
    dataPtr += 1;

    memcpy(dataPtr, data, dataSize);
    arFile->len += newEntry.size;
    return SISWA_SUCCESS;
}
siBool siswa_arEntryRemove(siArFile* arFile, const char* name) {
    return siswa_arEntryRemoveEx(arFile, name, strlen(name));
}
siBool siswa_arEntryRemoveEx(siArFile* arFile, const char* name, size_t nameLen) {
    size_t offset;
    siArEntry* entry;
    siByte* entryPtr;

    SISWA_ASSERT_NOT_NULL(name);

    entry = siswa_arEntryFindEx(*arFile, name, nameLen);
    entryPtr = (siByte*)entry;

    if (entry == NULL) {
        return SISWA_FAILURE;
    }
    offset = (size_t)entry - (size_t)arFile->data;

    arFile->len -= entry->size;
    memcpy(entryPtr, entryPtr + entry->size, arFile->len - offset);
    return SISWA_SUCCESS;
}
siBool siswa_arEntryUpdate(siArFile* arFile, const char* name, void* data,
        uint32_t dataSize) {
    return siswa_arEntryUpdateEx(arFile, name, strlen(name), data, dataSize);
}
siBool siswa_arEntryUpdateEx(siArFile* arFile, const char* name, size_t nameLen,
        void* data, uint32_t dataSize)  {
    size_t offset;
    siArEntry* entry;
    siByte* entryPtr;

    SISWA_ASSERT_NOT_NULL(name);
    SISWA_ASSERT_NOT_NULL(name);

    entry = siswa_arEntryFindEx(*arFile, name, nameLen);
    entryPtr = (siByte*)entry;

    if (entry == NULL) {
        return SISWA_FAILURE;
    }
    offset = (size_t)entry - (size_t)arFile->data;

    {
        int64_t oldSize = entry->size;

        entry->size = dataSize + nameLen + 1 + sizeof(siArEntry);
        entry->dataSize = dataSize;

        SISWA_ASSERT(offset + entry->size < arFile->cap,
                "Not enough space inside the buffer to update the entry.");

        /* Copy the data _after_ the entry so that it doesn't get overwritten. */
        memcpy(
            entryPtr + entry->size,
            entryPtr + (size_t)oldSize,
            arFile->len - offset - oldSize
        );
        /* Copy the new data into the entry. */
        memcpy(entryPtr + entry->offset, data, dataSize);

        arFile->len -= oldSize - (int64_t)entry->size;
    }


    return SISWA_SUCCESS;
}

siArFile siswa_arMerge(siArFile ars[2], void* outBuffer, size_t capacity) {
    return siswa_arMergeMul(ars, 2, outBuffer, capacity);
}
siArFile siswa_arMergeMul(siArFile* arrayOfArs, size_t arrayLen, void* outBuffer,
        size_t capacity) {
    char allocator[SISWA_DEFAULT_STACK_SIZE];

    siByte* ogBuffer = (siByte*)outBuffer;
    siByte* buffer = ogBuffer;
    size_t i;
    siHashTable* ht;
    uint32_t res;
    siArFile curAr;
    siArEntry* entry;
    size_t totalSize = sizeof(siArHeader);

    SISWA_ASSERT_NOT_NULL(arrayOfArs);
    SISWA_ASSERT_NOT_NULL(outBuffer);

    ht = si_hashtable_make_reserve(allocator, SISWA_DEFAULT_STACK_SIZE / sizeof(char*) - sizeof(siHashTable));

    {
        siArHeader header;
        header.unknown = 0;
        header.headerSizeof = sizeof(siArHeader);
        header.entrySizeof = sizeof(siArEntry);
        header.alignment = SISWA_DEFAULT_HEADER_ALIGNMENT;

        memcpy(buffer, &header, sizeof(header));
        buffer += sizeof(siArHeader);
    }

    for (i = 0; i < arrayLen; i++) {
        curAr = arrayOfArs[i];
        while ((entry = siswa_arEntryPoll(&curAr)) != NULL) {
            char* name = siswa_arEntryGetName(entry);
            res = si_hashtable_exists(ht, name);

            if (res == SISWA_FAILURE) {
                if (i != arrayLen - 1) {
                    si_hashtable_set(ht, name);
                }
                totalSize += entry->size;
                SISWA_ASSERT(capacity >= totalSize,
                    "Not enough space inside the buffer to merge all archive files"
                );
                memcpy(buffer, entry, entry->size);
                buffer += entry->size;
            }
        }
    }

    {
        siArFile ar = siswa_arMakeBufferEx(ogBuffer, totalSize, capacity);
        return ar;
    }
}

void siswa_arDecompress(siArFile* arl, siByte* out, size_t capacity, siBool freeCompData) {
    siswa_arlDecompress((siArlFile*)arl, out, capacity, freeCompData);
}
void siswa_arDecompressSegs(siArFile* arl, siByte *out, size_t capacity, siBool freeCompData) {
    siswa_arlDecompressSegs((siArlFile*)arl, out, capacity, freeCompData);
}
uint64_t siswa_arGetDecompressedSize(siArFile ar) {
    switch (ar.compression) {
        case SISWA_COMPRESSION_NONE: return ar.len;
        case SISWA_COMPRESSION_X: {
            uint64_t length = ((siXCompHeader*)ar.data)->uncompressedSize;
            if (siswa_isLittleEndian()) {
                length = siswa_swap64(length);
            }
            return length;
        }
        case SISWA_COMPRESSION_SEGS: {
            uint64_t data = ((siSegsHeader*)ar.data)->fullSize;
            if (siswa_isLittleEndian()) {
                data = siswa_swap32(data);
            }
            return data;
        }
    }
}

#ifndef SISWA_NO_STDLIB
void siswa_arFree(siArFile arFile) {
    free(arFile.data);
}
#endif


siArlFile siswa_arlMake(const char* path) {
    return siswa_arlMakeEx(path, 0);
}
#ifndef SISWA_NO_STDLIB
siArlFile siswa_arlMakeEx(const char* path, size_t additionalAllocSpace) {
    FILE* file;
    siByte* data;
    size_t dataLen;
    siArlFile arl;

    SISWA_ASSERT_NOT_NULL(path);

    file = fopen(path, "rb");
    SISWA_ASSERT_NOT_NULL(file);

    fseek(file, 0, SEEK_END);
    dataLen = ftell(file);
	rewind(file);

    data = (siByte*)malloc(dataLen + additionalAllocSpace);
    fread(data, dataLen, 1, file);

    arl = siswa_arlMakeBufferEx(data, dataLen, dataLen + additionalAllocSpace);
    fclose(file);

    return arl;
}
#endif
siArlFile siswa_arlMakeBuffer(void* data, size_t len) {
    return siswa_arlMakeBufferEx(data, len, len);
}
siArlFile siswa_arlMakeBufferEx(void* data, size_t len, size_t capacity) {
    siArlFile arl;
    uint32_t identifier;

    SISWA_ASSERT_NOT_NULL(data);
    SISWA_ASSERT(len <= capacity, "The length cannot be larger than the total capacity");

    identifier = *(uint32_t*)data;

    if (identifier == SISWA_IDENTIFIER_XCOMPRESSION) {
        arl.compression = SISWA_COMPRESSION_X;
    }
    else {
        SISWA_ASSERT(identifier == SISWA_IDENTIFIER_ARL2, "File is not a valid ARL2 file!");
        arl.compression = SISWA_COMPRESSION_NONE;
    }

    arl.data = (siByte*)data;
    arl.len = len;
    arl.cap = capacity;
    arl.curOffset = sizeof(siArHeader);

    return arl;
}

#ifndef SISWA_NO_STDLIB
siArlFile siswa_arlCreateArlContent(size_t capacity, size_t archiveCount) {
    size_t newCap =
        capacity + (sizeof(siArlHeader) - sizeof(uint32_t)) + archiveCount * sizeof(uint32_t);

    return siswa_arlCreateArlContentEx(
        malloc(newCap),
        newCap,
        archiveCount
    );
}
#endif
siArlFile siswa_arlCreateArlContentEx(void* buffer, size_t capacity, size_t archiveCount) {
    siArlHeader header;
    siArlFile arl;
    size_t length = sizeof(siArlHeader) - sizeof(uint32_t);

    SISWA_ASSERT_NOT_NULL(buffer);
    SISWA_ASSERT(
        capacity >= length + archiveCount * sizeof(uint32_t),
        "Capacity must be at least equal to or be higher than 'i"
        "(sizeof(siArlHeader) - sizeof(uint32_t)) + arrayLen * sizeof(uint32_t)'."
    );
    SISWA_ASSERT(archiveCount != 0, "Array length cannot be zero.");

    header.identifier = SISWA_IDENTIFIER_ARL2;
    header.archiveCount = archiveCount;
    memcpy(buffer, &header, length);
    memset((siByte*)buffer + length, 0, archiveCount * sizeof(uint32_t));
    length += archiveCount * sizeof(uint32_t);

    arl.data = (siByte*)buffer;
    arl.len = length;
    arl.cap = capacity;
    arl.compression = SISWA_COMPRESSION_NONE;
    arl.curOffset = length;

    return arl;
}
siArlFile siswa_arlCreateFromAr(siArFile arFile, void* outBuffer, size_t capacity) {
    siArlFile arl = siswa_arlCreateArlContentEx(outBuffer, capacity, 1);

    siArEntry* entry;
    while ((entry = siswa_arEntryPoll(&arFile))) {
        const char* name = siswa_arEntryGetName(entry);
        siswa_arlEntryAdd(&arl, name, 0);
    }

    return arl;
}
siArlFile siswa_arlCreateFromArMul(siArFile* arrayOfArs, size_t arrayLen,
        void* outBuffer, size_t capacity)  {
    siArlFile arl = siswa_arlCreateArlContentEx(outBuffer, capacity, arrayLen);

    size_t i;
    for (i = 0; i < arrayLen; i += 1) {
        siArEntry* entry;
        while ((entry = siswa_arEntryPoll(&arrayOfArs[i]))) {
            const char* name = siswa_arEntryGetName(entry);
            siswa_arlEntryAdd(&arl, name, i);
        }
    }

    return arl;
}

siArlHeader* siswa_arlGetHeader(siArlFile arlFile) {
    return (siArlHeader*)arlFile.data;
}
size_t siswa_arlGetEntryCount(siArlFile arFile) {
    size_t count = 0;
    while (siswa_arlEntryPoll(&arFile) != NULL) {
        count += 1;
    }
    return count;
}
size_t siswa_arlGetHeaderLength(siArlFile arlFile) {
    siArlHeader* header = siswa_arlGetHeader(arlFile);
    return (sizeof(siArlHeader) - sizeof(uint32_t)) + header->archiveCount * sizeof(uint32_t);
}

siArlEntry* siswa_arlEntryPoll(siArlFile* arlFile) {
    siArlEntry* entry;
    SISWA_ASSERT_NOT_NULL(arlFile);

    entry = (siArlEntry*)(arlFile->data + arlFile->curOffset);
    if (arlFile->curOffset >= arlFile->len) {
        siswa_arlOffsetReset(arlFile);
        return NULL;
    }

    arlFile->curOffset += entry->len + 1;
    return entry;
}
void siswa_arlOffsetReset(siArlFile* arlFile) {
    SISWA_ASSERT_NOT_NULL(arlFile);
    arlFile->curOffset = siswa_arlGetHeaderLength(*arlFile);
}
siArlEntry* siswa_arlEntryFind(siArlFile arlFile, const char* name) {
    return siswa_arlEntryFindEx(arlFile, name, strlen(name));
}
siArlEntry* siswa_arlEntryFindEx(siArlFile arFile, const char* name, size_t nameLen) {
    siArlEntry* entry;
    arFile.curOffset = siswa_arlGetHeaderLength(arFile);

    SISWA_ASSERT_NOT_NULL(name);

    while ((entry = siswa_arlEntryPoll(&arFile)) != NULL) {
        if (*name == *entry->string && strncmp(name + 1, entry->string + 1, nameLen - 1) == 0) {
            return entry;
        }
    }

    return NULL;
}

siBool siswa_arlEntryAdd(siArlFile* arlFile, const char* name, size_t archiveIndex) {
    return siswa_arlEntryAddEx(arlFile, name, strlen(name), archiveIndex);
}
siBool siswa_arlEntryAddEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
        size_t archiveIndex) {
    siByte* dataPtr;
    siArlHeader* header = siswa_arlGetHeader(*arlFile);
    size_t offset = siswa_arlGetHeaderLength(*arlFile);

    SISWA_ASSERT_NOT_NULL(arlFile);
    SISWA_ASSERT_NOT_NULL(name);
    SISWA_ASSERT(
        archiveIndex < header->archiveCount,
        "The provided archive index is too high than the linker's archive count."
    );

    {
        siArlEntry* entry;
        siArlFile tmpArFile = *arlFile;

        while ((entry = siswa_arlEntryPoll(&tmpArFile)) != NULL) {
            const char* entryName = entry->string;
            offset = tmpArFile.curOffset;

            if (*name == *entryName && strncmp(name + 1, entryName + 1, nameLen - 1) == 0) {
                return SISWA_FAILURE;
            }
        }
    }
    SISWA_ASSERT(offset + sizeof(uint8_t) + nameLen < arlFile->cap, "Not enough space inside the buffer to add a new entry.");
    dataPtr = arlFile->data + offset;
    *dataPtr = nameLen;
    dataPtr += 1;

    memcpy(dataPtr, name, nameLen);
    arlFile->len += sizeof(uint8_t) + nameLen;
    header->archiveSizes[archiveIndex] += sizeof(siArEntry) + nameLen + 1;

    return SISWA_SUCCESS;
}
siBool siswa_arlEntryRemove(siArlFile* arlFile, const char* name, size_t archiveIndex) {
    return siswa_arlEntryRemoveEx(arlFile, name, strlen(name), archiveIndex);
}
siBool siswa_arlEntryRemoveEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
        size_t archiveIndex) {
    size_t offset;
    siArlEntry* entry;
    siByte* entryPtr;
    siArlHeader* header = siswa_arlGetHeader(*arlFile);

    SISWA_ASSERT_NOT_NULL(name);
    SISWA_ASSERT(
        archiveIndex < header->archiveCount,
        "The provided archive index is too high than the linker's archive count."
    );

    entry = siswa_arlEntryFindEx(*arlFile, name, nameLen);
    entryPtr = (siByte*)entry;

    if (entry == NULL) {
        return SISWA_FAILURE;
    }
    offset = (size_t)entry - (size_t)arlFile->data;

    arlFile->len -= sizeof(uint8_t) + nameLen;
    memcpy(entryPtr, entryPtr + (sizeof(uint8_t) + nameLen), arlFile->len - offset);
    header->archiveSizes[archiveIndex] -= sizeof(uint8_t) + nameLen;

    return SISWA_SUCCESS;
}

siBool siswa_arlEntryUpdate(siArlFile* arlFile, const char* name, const char* newName,
        size_t archiveIndex) {
    return siswa_arlEntryUpdateEx(arlFile, name, strlen(name), newName, strlen(newName), archiveIndex);
}
siBool siswa_arlEntryUpdateEx(siArlFile* arlFile, const char* oldName, uint8_t oldNameLen,
        const char* newName, uint8_t newNameLen, size_t archiveIndex) {
    size_t offset;
    siArlEntry* entry;
    siByte* entryPtr;
    siArlHeader* header = siswa_arlGetHeader(*arlFile);

    SISWA_ASSERT_NOT_NULL(oldName);
    SISWA_ASSERT_NOT_NULL(newName);
    SISWA_ASSERT(
        archiveIndex < header->archiveCount,
        "The provided archive index is too high than the linker's archive count."
    );

    entry = siswa_arlEntryFindEx(*arlFile, oldName, oldNameLen);
    entryPtr = (siByte*)entry;

    if (entry == NULL) {
        return SISWA_FAILURE;
    }
    offset = (size_t)entry - (size_t)arlFile->data;

    {
        int64_t oldSize = sizeof(uint8_t) + entry->len;

        entry->len = newNameLen;

        SISWA_ASSERT(offset + sizeof(uint8_t) + newNameLen < arlFile->cap,
                "Not enough space inside the buffer to update the entry.");

        /* Copy the data _after_ the entry so that it doesn't get overwritten. */
        memcpy(
            entryPtr + sizeof(uint8_t) + newNameLen,
            entryPtr + (size_t)oldSize,
            arlFile->len - offset - oldSize
        );
        /* Copy the new name into the entry. */
        memcpy(entryPtr + 1, newName, newNameLen);

        arlFile->len -= oldSize - (int64_t)(sizeof(uint8_t) + newNameLen);
        header->archiveSizes[archiveIndex] -= oldSize - (int64_t)(sizeof(uint8_t) + newNameLen);
    }

    return SISWA_SUCCESS;
}

#ifndef SISWA_NO_DECOMPRESSION
void siswa_arlDecompress(siArlFile* arl, siByte* out, size_t capacity, siBool freeCompData) {
    SISWA_ASSERT_NOT_NULL(arl);
    SISWA_ASSERT_NOT_NULL(out);

    switch (arl->compression) {
        case SISWA_COMPRESSION_NONE: return ;
        case SISWA_COMPRESSION_SEGS: {
            siswa_arlDecompressSegs(arl, out, capacity, freeCompData);
            break;
        }
        case SISWA_COMPRESSION_X: {
            siswa_arlDecompressXComp(arl, out, capacity, freeCompData);
            break;
        }
    }
}
void siswa_arlDecompressSegs(siArlFile* arl, siByte* out, size_t capacity, siBool freeCompData) {
    SISWA_ASSERT_NOT_NULL(arl);
    SISWA_ASSERT_NOT_NULL(out);
    SISWA_ASSERT(arl->compression == SISWA_COMPRESSION_SEGS, "Wrong compression type.");

    {
        siSegsHeader* header = (siSegsHeader*)arl->data;
        uint32_t chunks = header->chunks;
        size_t fullSize = header->fullSize;
        if (siswa_isLittleEndian()) {
            chunks = siswa_swap16(chunks);
            fullSize = siswa_swap32(fullSize);
        }
        SISWA_ASSERT(
            capacity >= fullSize,
            "Capacity must be equal to or be higher than 'siswa_<ar/arl>GetDecompressedSize()'."
        );

        {
            siByte* curOutOffset = out;
            size_t baseOffset = sizeof(siSegsHeader) + (chunks * sizeof(siSegsEntry));
            siByte* curDataOffset =
                arl->data;

            siSegsEntry* entry = (siSegsEntry*)(header + 1);
            siSegsEntry* end = entry + chunks;
            size_t leftCapacity = capacity;

            for (; entry < end; entry += 1) {
                uint32_t size = entry->size;
                uint32_t zSize = entry->zSize;
                uint32_t offset = entry->offset;
                if (siswa_isLittleEndian()) {
                    size = siswa_swap16(size); /* TODO(EimaMei): swap64? */
                    zSize = siswa_swap16(zSize);
                    offset = siswa_swap32(offset);
                }
                offset -= 1;

                if (entry == (siSegsEntry*)(header + 1) && offset == 0) {
                    offset += baseOffset;
                }

                size += (size == 0) * 0xFFFF;
                if (size == zSize) {
                    memcpy(curOutOffset, curDataOffset + offset, size);
                }
                else {
                    siswa_decompressDeflate(curDataOffset + offset, zSize, curOutOffset, leftCapacity);
                }
                curOutOffset += size + 1;
                leftCapacity -= size;
            }
        }
        arl->len = fullSize;
        arl->cap = capacity;

        if (freeCompData) {
            free(arl->data);
        }
        arl->data = out;
        arl->compression = SISWA_COMPRESSION_NONE;
    }
}
void siswa_arlDecompressXComp(siArlFile* arl, siByte* out, size_t capacity, siBool freeCompData) {
    SISWA_ASSERT_NOT_NULL(arl);
    SISWA_ASSERT_NOT_NULL(out);
    SISWA_ASSERT(arl->compression == SISWA_COMPRESSION_X, "Wrong compression type.");

    {
        siXCompHeader* header = (siXCompHeader*)arl->data;
        uint32_t uncompBlockSize = header->uncompressedBlockSize;
        uint32_t compBlockMax = header->compressedBlockSizeMax;
        uint64_t fullSize = header->uncompressedSize;

        siByte* curDataOffset = (siByte*)(header + 1);
        if (siswa_isLittleEndian()) {
            uncompBlockSize = siswa_swap32(uncompBlockSize);
            compBlockMax = siswa_swap32(compBlockMax);
            fullSize = siswa_swap64(fullSize);
        }
        while (SISWA_TRUE) {
            uint32_t compressedBlockSize;
            uint32_t unknown;
            uint32_t uncompressedBlockSize;

            compressedBlockSize = *(uint32_t*)curDataOffset;
            curDataOffset += sizeof(uint32_t);

            unknown = *(uint8_t*)curDataOffset;
            curDataOffset += sizeof(uint8_t);

            if (unknown == 0) {
                break;
            }

            uncompressedBlockSize = *(uint16_t*)curDataOffset;
            curDataOffset += 20;


            if (siswa_isLittleEndian()) {
                compressedBlockSize = siswa_swap32(compressedBlockSize);
                uncompressedBlockSize = siswa_swap16(uncompressedBlockSize);
            }
            SISWA_ASSERT(uncompressedBlockSize == uncompressedBlockSize, "Cannot decompress this XCompressed file.");

            if (uncompressedBlockSize == uncompBlockSize) {
                memcpy(out, curDataOffset, uncompressedBlockSize);
            }

            curDataOffset += compressedBlockSize;
        }
        arl->len = fullSize;
        arl->cap = capacity;

        if (freeCompData) {
            free(arl->data);
        }
        arl->data = out;
        arl->compression = SISWA_COMPRESSION_NONE;
    }
}

uint64_t siswa_arlGetDecompressedSize(siArlFile arl) {
    switch (arl.compression) {
        case SISWA_COMPRESSION_NONE: return arl.len;
        case SISWA_COMPRESSION_X: {
            uint64_t length = ((siXCompHeader*)arl.data)->uncompressedSize;
            if (siswa_isLittleEndian()) {
                length = siswa_swap64(length);
            }
            return length;
        }
        case SISWA_COMPRESSION_SEGS: {
            uint32_t data = ((siSegsHeader*)arl.data)->fullSize;
            if (siswa_isLittleEndian()) {
                data = siswa_swap32(data);
            }
            return data;
        }
    }
}

#endif

#ifndef SISWA_NO_STDLIB
void siswa_arlFree(siArlFile arlFile) {
    free(arlFile.data);
}
#endif


#if !defined(SISWA_NO_DECOMPRESSION) && !defined(SISWA_DEFINE_CUSTOM_DEFLATE_DECOMPRESSION)

#define SINFL_PRE_TBL_SIZE 128
#define SINFL_LIT_TBL_SIZE 1334
#define SINFL_OFF_TBL_SIZE 402

typedef struct {
    uint8_t* bitptr;
    size_t bitbuf;
    int32_t bitcnt;

    uint32_t lits[SINFL_LIT_TBL_SIZE];
    uint32_t dsts[SINFL_OFF_TBL_SIZE];
} sinfl;

#if defined(__GNUC__) || defined(__clang__)
    #define sinfl_likely(x)       __builtin_expect((x),1)
    #define sinfl_unlikely(x)     __builtin_expect((x),0)
#else
    #define sinfl_likely(x)       (x)
    #define sinfl_unlikely(x)     (x)
#endif

#ifndef SINFL_NO_SIMD
#if defined(__x86_64__) || defined(_WIN32) || defined(_WIN64)
    #include <emmintrin.h>
    #define sinfl_char16 __m128i
    #define sinfl_char16_ld(p) _mm_loadu_si128((const __m128i *)(void*)(p))
    #define sinfl_char16_str(d,v)  _mm_storeu_si128((__m128i*)(void*)(d), v)
    #define sinfl_char16_char(c) _mm_set1_epi8(c)
#elif defined(__arm__) || defined(__aarch64__)
    #include <arm_neon.h>
    #define sinfl_char16 uint8x16_t
    #define sinfl_char16_ld(p) vld1q_u8((const unsigned char*)(p))
    #define sinfl_char16_str(d,v) vst1q_u8((unsigned char*)(d), v)
    #define sinfl_char16_char(c) vdupq_n_u8(c)
#else
    #define SINFL_NO_SIMD
#endif
#endif

static
int32_t sinfl_bsr(uint32_t n) {
#ifdef _MSC_VER
    _BitScanReverse(&n, n);
    return n;
#elif defined(__GNUC__) || defined(__clang__)
    return 31 - __builtin_clz(n);
#else
    /* TODO(EimaMei): add a cross-compiler version */
#endif
}
#define sinfl_read64(ptr) (*(uint64_t*)ptr)
#define sinfl_copy64(dst, src) sinfl_read64(dst) = sinfl_read64(src); dst += 8; src += 8

#ifndef SINFL_NO_SIMD
static
siByte* sinfl_write128(siByte* dst, sinfl_char16 w) {
  sinfl_char16_str(dst, w);
  return dst + 8;
}
static
void sinfl_copy128(unsigned char** dst, unsigned char** src) {
    sinfl_char16 n = sinfl_char16_ld(*src);
    sinfl_char16_str(*dst, n);
    *dst += 16;
    *src += 16;
}
#endif
static
void sinfl_refill(sinfl* s) {
    s->bitbuf |= sinfl_read64(s->bitptr) << s->bitcnt;
    s->bitptr += (63 - s->bitcnt) >> 3;
    s->bitcnt |= 56; /* bitcount in range [56,63] */
}
static
size_t sinfl_peek(sinfl* s, int32_t cnt) {
    assert(cnt >= 0 && cnt <= 56);
    assert(cnt <= s->bitcnt);
    return s->bitbuf & (((uint64_t)1 << cnt) - 1);
}
static
void sinfl_eat(sinfl* s, int32_t cnt) {
    assert(cnt <= s->bitcnt);
    s->bitbuf >>= cnt;
    s->bitcnt -= cnt;
}
static
int32_t sinfl__get(sinfl* s, int32_t cnt) {
    int32_t res = sinfl_peek(s, cnt);
    sinfl_eat(s, cnt);
    return res;
}
static
uint32_t sinfl_get(sinfl* s, int32_t cnt) {
    sinfl_refill(s);
    return sinfl__get(s, cnt);
}
typedef struct {
    size_t len;
    size_t cnt;
    uint32_t word;
    uint16_t* sorted;
} sinfl_gen;


static
int32_t sinfl_build_tbl(sinfl_gen* gen, uint32_t* tbl, uint32_t tbl_bits,
        uint32_t* cnt) {
    size_t tbl_end = 0;
    while (!(gen->cnt = cnt[gen->len])) {
        gen->len += 1;
    }
    tbl_end = 1 << gen->len;
    while (gen->len <= tbl_bits) {
        do {
            unsigned bit = 0;
            tbl[gen->word] = (*gen->sorted++ << 16) | gen->len;
            if (gen->word == tbl_end - 1) {
                while (gen->len < tbl_bits) {
                    memcpy(&tbl[tbl_end], tbl, (size_t)tbl_end * sizeof(tbl[0]));
                    tbl_end <<= 1;
                    gen->len += 1;
                }
                return 1;
            }
            bit = 1 << sinfl_bsr((uint32_t)(gen->word ^ (tbl_end - 1)));
            gen->word &= bit - 1;
            gen->word |= bit;
        } while (--gen->cnt);
        do {
            if (++gen->len <= tbl_bits) {
                memcpy(&tbl[tbl_end], tbl, (size_t)tbl_end * sizeof(tbl[0]));
                tbl_end <<= 1;
            }
        } while (!(gen->cnt = cnt[gen->len]));
  }
  return 0;
}
static
void sinfl_build_subtbl(sinfl_gen *gen, uint32_t* tbl, uint32_t tbl_bits, uint32_t* cnt) {
    uint32_t sub_bits = 0;
    size_t sub_start = 0;
    size_t sub_prefix = (size_t)0 - 1;
    size_t tbl_end = 1 << tbl_bits;


    while (1) {
        uint32_t entry;
        size_t bit, stride, i;

        /* start new sub-table */
        if ((gen->word & ((1 << tbl_bits) - 1)) != sub_prefix) {
            int32_t used = 0;
            sub_prefix = gen->word & ((1 << tbl_bits)-1);
            sub_start = tbl_end;
            sub_bits = gen->len - tbl_bits;
            used = gen->cnt;

            while (used < (1 << sub_bits)) {
                sub_bits++;
                used = (used << 1) + cnt[tbl_bits + sub_bits];
            }
            tbl_end = sub_start + (1 << sub_bits);
            tbl[sub_prefix] = (sub_start << 16) | 0x10 | (sub_bits & 0xf);
        }

        /* fill sub-table */
        entry = (*gen->sorted << 16) | ((gen->len - tbl_bits) & 0xf);
        gen->sorted++;
        i = sub_start + (gen->word >> tbl_bits);
        stride = 1 << (gen->len - tbl_bits);
        do {
            tbl[i] = entry;
            i += stride;
        } while (i < tbl_end);

        if (gen->word == (1 << gen->len)-1) {
            return;
        }

        bit = 1 << sinfl_bsr(gen->word ^ ((1 << gen->len) - 1));
        gen->word &= bit - 1;
        gen->word |= bit;
        gen->cnt -= 1;
        while (!gen->cnt) {
            gen->cnt = cnt[++gen->len];
        }
    }
}
static
void sinfl_build(uint32_t* tbl, siByte* lens, uint32_t tbl_bits, size_t maxlen,
        size_t symcnt) {
    size_t i, used = 0;
    uint16_t sort[288];
    uint32_t cnt[16] = {0}, off[16]= {0};
    sinfl_gen gen = {0};
    gen.sorted = sort;
    gen.len = 1;

    for (i = 0; i < symcnt; i += 1) {
        cnt[lens[i]] += 1;
    }
    off[1] = cnt[0];

    for (i = 1; i < maxlen; i += 1) {
        off[i + 1] = off[i] + cnt[i];
        used = (used << 1) + cnt[i];
    }
    used = (used << 1) + cnt[i];

    for (i = 0; i < symcnt; i += 1) {
        gen.sorted[off[lens[i]]++] = (int16_t)i;
    }
    gen.sorted += off[0];

    if (used < (1 << maxlen)) {
        for (i = 0; i < 1 << tbl_bits; i += 1) {
            tbl[i] = (0 << 16u) | 1;
        }
        return;
    }

    if (!sinfl_build_tbl(&gen, tbl, tbl_bits, cnt)) {
        sinfl_build_subtbl(&gen, tbl, tbl_bits, cnt);
    }
}
static
int32_t sinfl_decode(sinfl* s, uint32_t* tbl, size_t bit_len) {
    size_t idx = sinfl_peek(s, bit_len);
    uint32_t key = tbl[idx];

    if (key & 0x10) {
        /* sub-table lookup */
        size_t len = key & 0x0f;
        sinfl_eat(s, bit_len);
        idx = sinfl_peek(s, len);
        key = tbl[((key >> 16) & 0xffff) + (unsigned)idx];
    }
    sinfl_eat(s, key & 0x0f);

    return (key >> 16) & 0x0fff;
}
extern
size_t siswa_decompressDeflate(siByte* data, size_t length, siByte* out, size_t capacity) {
#if 1
    static uint8_t order[] = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
    };
    static uint16_t dbase[30 + 2] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
        769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
    };
    static siByte dbits[30 + 2] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10,
        11, 11, 12, 12, 13, 13, 0, 0
    };
    static uint16_t lbase[29 + 2] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
        67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
    };
    static uint8_t lbits[29 + 2] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4,
        5, 5, 5, 5, 0, 0, 0
    };
#endif

    siByte* capacityEnd = out + capacity;
    siByte* offsetEnd = data + length;
    siByte* offset = out;

    typedef enum { HDR, STORED, FIXED, DYNAMIC, BULK} sinfl_states;
    sinfl_states state = HDR;
    sinfl s = {0};
    int32_t last = 0;

    s.bitptr = data;
    while (1) {
        switch (state) {
            case HDR: {
                /* block header */
                int32_t type;
                sinfl_refill(&s);
                last = sinfl__get(&s, 1);
                type = sinfl__get(&s, 2);

                switch (type) {
                    case 0x00: state = STORED; break;
                    case 0x01: state = FIXED; break;
                    case 0x02: state = DYNAMIC; break;
                    default: return (out - offset);
                }
                break;
            }
            case STORED: {
                /* uncompressed block */
                size_t len, nlen;
                sinfl__get(&s, s.bitcnt & 7);
                len = (uint16_t)sinfl__get(&s,16);
                nlen = (uint16_t)sinfl__get(&s,16);
                s.bitptr -= s.bitcnt / 8;
                s.bitbuf = s.bitcnt = 0;

                if (
                    ((uint16_t)len != (uint16_t)~nlen)
                    || len > (size_t)(offsetEnd - s.bitptr)
                    || !len
                ) {
                    return (out - offset);
                }

                memcpy(out, s.bitptr, len);
                s.bitptr += len;
                out += len;

                if (last) {
                    return (out - offset);
                }
                state = HDR;
                break;
            }
            case FIXED: {
              /* fixed huffman codes */
                uint8_t lens[288 + 32]; /* TODO(EimaMei): Imrpove performance? */
                size_t n;
                for (n = 0; n <= 143; n++) lens[n] = 8;
                for (n = 144; n <= 255; n++) lens[n] = 9;
                for (n = 256; n <= 279; n++) lens[n] = 7;
                for (n = 280; n <= 287; n++) lens[n] = 8;
                for (n = 0; n < 32; n++) lens[288+n] = 5;

                /* build lit/dist tables */
                sinfl_build(s.lits, lens, 10, 15, 288);
                sinfl_build(s.dsts, lens + 288, 8, 15, 32);
                state = BULK;
                break;
            }
            case DYNAMIC: {
              /* dynamic huffman codes */
                size_t n, i;
                uint32_t hlens[SINFL_PRE_TBL_SIZE];
                uint8_t nlens[19] = {0};
                uint8_t lens[288 + 32];

                sinfl_refill(&s);
                {
                    size_t nlit = 257 + sinfl__get(&s,5);
                    size_t ndist = 1 + sinfl__get(&s,5);
                    size_t nlen = 4 + sinfl__get(&s,4);

                    for (n = 0; n < nlen; n++) {
                        nlens[order[n]] = (unsigned char)sinfl_get(&s,3);
                    }
                    sinfl_build(hlens, nlens, 7, 7, 19);

                    /* decode code lengths */
                    for (n = 0; n < nlit + ndist;) {
                        int32_t sym = 0;
                        sinfl_refill(&s);
                        sym = sinfl_decode(&s, hlens, 7);
                        switch (sym) {
                            case 16: {
                                i = 3 + sinfl_get(&s, 2);
                                while (i) {
                                    lens[n] = lens[n - 1];
                                    i -= 1;
                                    n += 1;
                                }
                                break;
                            }
                            case 17: {
                                i = 3 + sinfl_get(&s, 3);
                                while (i) {
                                    lens[n] = 0;
                                    i -= 1;
                                    n += 1;
                                }
                                break;
                            }
                            case 18: { /* TODO(EimaMei): test if just doing memset is faster. */
                                i = 11 + sinfl_get(&s, 7);
                                while (i) {
                                    lens[n] = 0;
                                    i -= 1;
                                    n += 1;
                                }
                                break;
                            }
                            default: lens[n++] = (uint8_t)sym; break;
                        }
                    }
                    /* build lit/dist tables */
                    sinfl_build(s.lits, lens, 10, 15, nlit);
                    sinfl_build(s.dsts, lens + nlit, 8, 15, ndist);
                    state = BULK;
                }
                break;
            }
            case BULK: {
                /* decompress block */
                while (1) {
                    int sym;
                    sinfl_refill(&s);
                    sym = sinfl_decode(&s, s.lits, 10);
                    if (sym < 256) {
                        /* literal */
                        if (sinfl_unlikely(out >= capacityEnd)) {
                            return out - offset;
                        }
                        *out++ = (uint8_t)sym;
                        sym = sinfl_decode(&s, s.lits, 10);
                        if (sym < 256) {
                            *out++ = (uint8_t)sym;
                            continue;
                        }
                    }
                    if (sinfl_unlikely(sym == 256)) {
                        /* end of block */
                        if (last) {
                            return out - offset;
                        }
                        state = HDR;
                        break;
                    }
                    /* match */
                    if (sym >= 286) {
                        /* length codes 286 and 287 must not appear in compressed data */
                        return out - offset;
                    }
                    sym -= 257;
                    {
                        int32_t len = sinfl__get(&s, lbits[sym]) + lbase[sym];
                        int32_t dsym = sinfl_decode(&s, s.dsts, 8);
                        int32_t offs = sinfl__get(&s, dbits[dsym]) + dbase[dsym];
                        siByte* dst = out;
                        siByte* src =  out - offs;
                        if (sinfl_unlikely(offs >  out - offset)) {
                            return out - offset;
                        }
                        out = out + len;

#ifndef SINFL_NO_SIMD
                        if (sinfl_likely(capacityEnd - out >= 16 * 3)) {
                            if (offs >= 16) {
                                /* simd copy match */
                                sinfl_copy128(&dst, &src);
                                sinfl_copy128(&dst, &src);
                                do {
                                    sinfl_copy128(&dst, &src);
                                } while (dst < out);
                            }
                            else if (offs >= 8) {
                                /* word copy match */
                                sinfl_copy64(dst, src);
                                sinfl_copy64(dst, src);
                                do {
                                    sinfl_copy64(dst, src);
                                } while (dst < out);
                            }
                            else if (offs == 1) {
                                /* rle match copying */
                                sinfl_char16 w = sinfl_char16_char(src[0]);
                                dst = sinfl_write128(dst, w);
                                dst = sinfl_write128(dst, w);
                                do {
                                    dst = sinfl_write128(dst, w);
                                } while (dst < out);
                            }
                            else {
                                /* byte copy match */
                                *dst++ = *src++;
                                *dst++ = *src++;
                                do {
                                    *dst++ = *src++;
                                } while (dst < out);
                            }
                        }
#else
                     if (sinfl_likely(oe - out >= 3 * 8 - 3)) {
                        if (offs >= 8) {
                            /* word copy match */
                            sinfl_copy64(dst, src);
                            sinfl_copy64(dst, src);
                            do {
                                sinfl_copy64(dst, src);
                            } while (dst < out);
                        }
                        else if (offs == 1) {
                            /* rle match copying */
                            unsigned int c = src[0];
                            unsigned int hw = (c << 24u) | (c << 16u) | (c << 8u) | (unsigned)c;
                            unsigned long long w = (unsigned long long)hw << 32llu | hw;
                            dst = sinfl_write64(dst, w);
                            dst = sinfl_write64(dst, w);
                            do {
                                dst = sinfl_write64(dst, w);
                            } while (dst < out);
                        }
                        else {
                            /* byte copy match */
                            *dst++ = *src++;
                            *dst++ = *src++;
                            do {
                                *dst++ = *src++;
                            } while (dst < out);
                        }
                    }
#endif
                        else {
                            *dst++ = *src++;
                            *dst++ = *src++;
                            do {
                                *dst++ = *src++;
                            } while (dst < out);
                        }
                    }
                }
                break;
            }
        }
    }
    return (out - offset);
}
#endif

#endif

#ifdef __cplusplus
}
#endif

#ifdef SISWA_USE_PRAGMA_PACK
    #pragma pack(pop)
#endif

#endif /* SISWA_INCLUDE_SISWA_H */

/*
------------------------------------------------------------------------------
Copyright (C) 2023 EimaMei

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
------------------------------------------------------------------------------
*/
