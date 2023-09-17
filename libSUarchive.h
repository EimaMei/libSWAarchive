/*
libSUarchive - a light, fast and portable library for handling Sonic Unleashed's archive file formats (.ar/.arl).
===========================================================================
	- YOU MUST DEFINE 'SISWA_ARCHIVE_IMPLEMENTATION' in EXACTLY _one_ file that includes
	this header, BEFORE the include like this:

		#define SISWA_ARCHIVE_IMPLEMENTATION
		#include "libSUarchive.h"
	- All other files should just include the library without the #define macro.

    - For "release mode", you can disable safety assertions that siswa does by
    defining 'SI_UNDEFINE_ASSERT' before including the file. For a custom assertion,
    you can define a 'SISWA_ASSERT(x, msg)' beforehand.

    - By default libSUarchive uses 'stdint.h' for the standard types. If you wish
    to NOT use it, or you're using pure C89, then you can define 'SISWA_DEFINE_CUSTOM_INT_TYPES'
    before the include to disable that. As the name entails, you must now define
    your own int types as a result for the library to work. These include:
        - uint8_t - 1 byte, unsigned.
        - uint16_t - 2 bytes, unsigned.
        - uint32_t and int32_t - 4 bytes, both signed and unsigned.
        - uint64_t and int64_t - 8 bytes, both signed and unsigned.
    - If the 'sizeof' for one of the above mentioned types doesn't match, then
    the header won't compile and complain about the invalid size.

===========================================================================

LICENSE
	- This software is licensed under the zlib license (see the LICENSE in the
    bottom of the file).

WARNING
	- This library is _slightly_ experimental and features may not work as
	expected.
	- This also means that many functions are not documented.
*/

#ifndef SISWA_INCLUDE_SISWA_H
#define SISWA_INCLUDE_SISWA_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#if !defined(SISWA_DEFINE_CUSTOM_INT_TYPES)
    #include <stdint.h>
#endif

#if !defined(SISWA_ASSERT) && !defined(SI_UNDEFINE_ASSERT)
    #define SISWA_ASSERT(x, msg) \
        if (!(x)) { \
            fprintf(stderr, "Assertion \""  #x "\" failed at \"%s:%d\": %s\n", __FILE__, __LINE__, msg); \
            exit(1); \
        } do {} while (0)
#elif !defined(SISWA_ASSERT) && defined(SI_UNDEFINE_ASSERT)
    #define SISWA_ASSERT(x, msg)  (void*)(x); (void*)(msg)
#endif

#define SISWA_ASSERT_NOT_NULL(ptr) SISWA_ASSERT((ptr) != NULL, #ptr " must not be NULL.")

#define SISWA_STATIC_ASSERT2(cond, msg)  typedef char static_assertion_##msg[(!!(cond)) * 2 - 1] /* NOTE(EimaMei): This is absolutely stupid but somehow it works so who cares, really? */
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
typedef int32_t siResult;


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

#define SISWA_UNSPECIFIED_LEN 1

#define SISWA_IDENTIFIER_XCOMPRESSION 0xEE12F50F
#define SISWA_IDENTIFIER_ARL2 0x324C5241


typedef enum {
    SI_COMPRESS_NONE,
    SI_COMPRESS_XCOMPRESSION,
    SI_COMPRESS_SEGS
} siCompressType;


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
    /* The high byte of the file date. Usually 0. */
    uint32_t filedateHigh;
    /* The low byte of the file data. Usually 0. */
    uint32_t filedateLow;
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
    siCompressType compressed;
    /* Current entry offset, modified by 'siswa_xEntryPoll'. Should not be modified otherwise.*/
    size_t curOffset;
} siArFile;


/* Creates a 'siArFile' structure from an '.ar' file. Must not be compressed. */
siArFile siswa_arMake(const char* path);
/* Creates a 'siArFile' structure from an '.ar' file and adds additional space to the capacity. Must not be compressed. */
siArFile siswa_arMakeEx(const char* path, size_t additionalAllocSpace);
/* Creates a 'siArFile' structure from an archive file's content in memory. Must not be compressed. */
siArFile siswa_arMakeBuffer(void* data, size_t len);
/* Creates a 'siArFile' structure from an archive file's content in memory, while also setting the capacity. Must not be compressed. */
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
/* Finds an entry matching the provided name. Returns NULL if the entry doesn't exist. */
siArEntry* siswa_arEntryFind(siArFile arFile, const char* name);
/* Finds an entry matching the provided name with length. Returns NULL if the entry doesn't exist. */
siArEntry* siswa_arEntryFindEx(siArFile arFile, const char* name, size_t nameLen);

/* Gets the name of the provided entry. */
char* siswa_arEntryGetName(siArEntry* entry);
/* Gets the data of the provided entry. */
void* siswa_arEntryGetData(siArEntry* entry);

/* Adds a new entry in the archive. Fails if the entry name already exists or the capacity is too low. */
siResult siswa_arEntryAdd(siArFile* arFile, const char* name, void* data,
        uint32_t dataSize);
/* Adds a new entry in the archive. Fails if the entry name already exists or the capacity is too low. */
siResult siswa_arEntryAddEx(siArFile* arFile, const char* name, size_t nameLen,
        void* data, uint32_t dataSize);
/* Removes an entry in the archive. Fails if the entry doesn't exist. */
siResult siswa_arEntryRemove(siArFile* arFile, const char* name);
/* Removes an entry in the archive. Fails if the entry doesn't exist. */
siResult siswa_arEntryRemoveEx(siArFile* arFile, const char* name, size_t nameLen);
/* Updates the entry inside the archive. Fails if the entry doesn't exist. */
siResult siswa_arEntryUpdate(siArFile* arFile, const char* name, void* data,
        uint32_t dataSize);
/* Updates the entry inside the archive. Fails if the entry doesn't exist. */
siResult siswa_arEntryUpdateEx(siArFile* arFile, const char* name, size_t nameLen,
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


/* Creates a 'siArlFile' structure from an '.arl' file. Must not be compressed. */
siArlFile siswa_arlMake(const char* path);
/* Creates a 'siArlFile' structure from an '.arl' file and adds additional space
 * to the capacity. Must not be compressed. */
siArlFile siswa_arlMakeEx(const char* path, size_t additionalAllocSpace);
/* Creates a 'siArlFile' structure from an archive linker's content in memory.
 * Must not be compressed. */
siArlFile siswa_arlMakeBuffer(void* data, size_t len);
/* Creates a 'siArlFile' structure from an archive linker's content in memory,
 * while also setting the capacity. Must not be compressed. */
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
/* Finds an entry matching the provided name. Returns NULL if the entry doesn't
 * exist. */
siArlEntry* siswa_arlEntryFind(siArlFile arlfile, const char* name);
/* Finds an entry matching the provided name with length. Returns NULL if the entry
 * doesn't exist. */
siArlEntry* siswa_arlEntryFindEx(siArlFile arlfile, const char* name, size_t nameLen);

/* Adds a new entry in the archive linker. Fails if the entry name already exists
 * or the capacity is too low. */
siResult siswa_arlEntryAdd(siArlFile* arlFile, const char* name, size_t archiveIndex);
/* Adds a new entry in the archive linker. Fails if the entry name already exists
 * or the capacity is too low. */
siResult siswa_arlEntryAddEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
        size_t archiveIndex);
/* Removes an entry in the archive linker. Fails if the entry doesn't exist. */
siResult siswa_arlEntryRemove(siArlFile* arlFile, const char* name, size_t archiveIndex);
/* Removes an entry in the archive linker. Fails if the entry doesn't exist. */
siResult siswa_arlEntryRemoveEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
        size_t archiveIndex);
/* Updates the entry inside the archive linker. Fails if the entry doesn't exist. */
siResult siswa_arlEntryUpdate(siArlFile* arlFile, const char* name, const char* newName,
        size_t archiveIndex);
/* Updates the entry inside the archive linker. Fails if the entry doesn't exist. */
siResult siswa_arlEntryUpdateEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
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

/* Frees arlFile.buffer. Same as doing free(arlFile.data) */
void siswa_arlFree(siArlFile arlFile);


#if 0
typedef struct { /* NOTE(EimaMei): All credit goes to https://github.com/mistydemeo/quickbms/blob/master/unz.c */
    uint32_t identifier;
    uint16_t version;
    uint16_t reserved;
    uint32_t contextFlags;
    uint32_t flags;
    uint32_t windowSize;
    uint32_t compressionPartitionSize;
    uint32_t uncompressedSizeHigh;
    uint32_t uncompressedSizeLow;
    uint32_t compressedSizeHigh;
    uint32_t compressedSizeLow;
    uint32_t uncompressedBlockSize;
    uint32_t compressedBlockSizeMax;
} siXcompressNative;
#endif

#if defined(SISWA_ARCHIVE_IMPLEMENTATION)

#if 1 /* NOTE(EimaMei): Internal hashing stuff, can ignore */
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
        ar.compressed = SI_COMPRESS_XCOMPRESSION;
    }
    else {
        SISWA_ASSERT(identifier != SISWA_IDENTIFIER_ARL2, "Use 'siswa_arlFileOpen' for ARL2 files!");
        ar.compressed = SI_COMPRESS_NONE;
    }

    ar.data = (siByte*)data;
    ar.len = len;
    ar.cap = capacity;
    ar.curOffset = sizeof(siArHeader);

    return ar;
}

siArFile siswa_arCreateArContent(size_t capacity) {
    return siswa_arCreateArContentEx(malloc(capacity + sizeof(siArHeader)), capacity);
}
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
    ar.compressed = SI_COMPRESS_NONE;
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
        arFile->curOffset = sizeof(siArHeader);
        return NULL;
    }

    arFile->curOffset += entry->size;
    return entry;
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

siResult siswa_arEntryAdd(siArFile* arFile, const char* name, void* data, uint32_t dataSize) {
    return siswa_arEntryAddEx(arFile, name, strlen(name), data, dataSize);
}
siResult siswa_arEntryAddEx(siArFile* arFile, const char* name, size_t nameLen, void* data, uint32_t dataSize) {
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
    newEntry.filedateHigh = 0;
    newEntry.filedateLow = 0;

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
siResult siswa_arEntryRemove(siArFile* arFile, const char* name) {
    return siswa_arEntryRemoveEx(arFile, name, strlen(name));
}
siResult siswa_arEntryRemoveEx(siArFile* arFile, const char* name, size_t nameLen) {
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
siResult siswa_arEntryUpdate(siArFile* arFile, const char* name, void* data,
        uint32_t dataSize) {
    return siswa_arEntryUpdateEx(arFile, name, strlen(name), data, dataSize);
}
siResult siswa_arEntryUpdateEx(siArFile* arFile, const char* name, size_t nameLen,
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

void siswa_arFree(siArFile arFile) {
    free(arFile.data);
}


siArlFile siswa_arlMake(const char* path) {
    return siswa_arlMakeEx(path, 0);
}
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
        arl.compressed = SI_COMPRESS_XCOMPRESSION;
    }
    else {
        SISWA_ASSERT(identifier == SISWA_IDENTIFIER_ARL2, "File is not a valid ARL2 file!");
        arl.compressed = SI_COMPRESS_NONE;
    }

    arl.data = (siByte*)data;
    arl.len = len;
    arl.cap = capacity;
    arl.curOffset = sizeof(siArHeader);

    return arl;
}

siArlFile siswa_arlCreateArlContent(size_t capacity, size_t archiveCount) {
    size_t newCap =
        capacity + (sizeof(siArlHeader) - sizeof(uint32_t)) + archiveCount * sizeof(uint32_t);

    return siswa_arlCreateArlContentEx(
        malloc(newCap),
        newCap,
        archiveCount
    );
}
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
    arl.compressed = SI_COMPRESS_NONE;
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
        arlFile->curOffset = siswa_arlGetHeaderLength(*arlFile);
        return NULL;
    }

    arlFile->curOffset += entry->len + 1;
    return entry;
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

siResult siswa_arlEntryAdd(siArlFile* arlFile, const char* name, size_t archiveIndex) {
    return siswa_arlEntryAddEx(arlFile, name, strlen(name), archiveIndex);
}
siResult siswa_arlEntryAddEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
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
siResult siswa_arlEntryRemove(siArlFile* arlFile, const char* name, size_t archiveIndex) {
    return siswa_arlEntryRemoveEx(arlFile, name, strlen(name), archiveIndex);
}
siResult siswa_arlEntryRemoveEx(siArlFile* arlFile, const char* name, uint8_t nameLen,
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

siResult siswa_arlEntryUpdate(siArlFile* arlFile, const char* name, const char* newName,
        size_t archiveIndex) {
    return siswa_arlEntryUpdateEx(arlFile, name, strlen(name), newName, strlen(newName), archiveIndex);
}
siResult siswa_arlEntryUpdateEx(siArlFile* arlFile, const char* oldName, uint8_t oldNameLen,
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

void siswa_arlFree(siArlFile arlFile) {
    free(arlFile.data);
}

#endif

#if defined(__cplusplus)
}
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
