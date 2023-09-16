/*
libSUarchive - a light, fast and portable library for handling Sonic Unleashed's archive file format.
===========================================================================
	- YOU MUST DEFINE 'SISWA_ARCHIVE_IMPLEMENTATION' in EXACTLY _one_ file that includes
	this header, BEFORE the include like this:

		#define SISWA_ARCHIVE_IMPLEMENTATION
		#include "libSUarchive.h"
	- All other files should just include the library without the #define macro.

    - For "release mode", you can disable safety assertions that siswa does by
    defining 'SI_UNDEFINE_ASSERT' before including the file. For a custom assertion,
    you can define a 'SI_ASSERT(x, msg)' beforehand.

    - By default libSUarchive uses 'stdin.h' for the standard types. If you wish
    to NOT use it, or you're using pure C89, then you can define 'SI_DEFINE_CUSTOM_INTTYPES'
    before the include to disable that. As the name entails, you must now define
    your own int types as a result for the library to work. These include:
        - uint8_t - 1 byte.
        - uint16_t - 2 bytes.
        - uint32_t and int32_t - 4 bytes.
        - uint64_t - 8 bytes.
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


#if !defined(SI_DEFINE_CUSTOM_INTTYPES)
    #include <stdint.h>
#endif

#if !defined(SI_ASSERT) && !defined(SI_UNDEFINE_ASSERT)
    #define SI_ASSERT(x, msg) \
        if (!(x)) { \
            fprintf(stderr, "Assertion \""  #x "\" failed at \"%s:%d\": %s\n", __FILE__, __LINE__, msg); \
            exit(1); \
        } do {} while (0)
#elif !defined(SI_ASSERT) && defined(SI_UNDEFINE_ASSERT)
    #define SI_ASSERT(x, msg) do {} while (0)
#endif

#define SI_ASSERT_NOT_NULL(ptr) SI_ASSERT((ptr) != NULL, #ptr " must not be NULL.")

#define SI_STATIC_ASSERT2(cond, msg)  typedef char static_assertion_##msg[(!!(cond)) * 2 - 1] /* NOTE(EimaMei): This is absolutely stupid but somehow it works so who cares, really? */
#define SI_STATIC_ASSERT1(cond, line) SI_STATIC_ASSERT2(cond, line)
#define SI_STATIC_ASSERT(cond)        SI_STATIC_ASSERT1(cond, __LINE__)


SI_STATIC_ASSERT(sizeof(uint8_t) == 1);
SI_STATIC_ASSERT(sizeof(uint16_t) == 2);
SI_STATIC_ASSERT(sizeof(int32_t) == 4);
SI_STATIC_ASSERT(sizeof(uint32_t) == 4);
SI_STATIC_ASSERT(sizeof(uint64_t) == 8);
SI_STATIC_ASSERT(sizeof(size_t) == sizeof(void*));

#ifndef siByte
    typedef uint8_t siByte;
#endif
typedef int32_t siResult;

#define SI_DEFAULT_STACK_SIZE 8 * 1024 /* NOTE(EimaMei): Used for storing string pointers.
                                          The higher the value, the less likely a hash collision will happen.
                                          8kb is a good middleground balance on the major OSses, but probably not
                                          for actual embedded systems.*/

#define SI_SUCCESS 1
#define SI_FAILURE 0

#define SI_UNSPECIFIED_LEN 1

#define SI_IDENTIFIER_XCOMPRESSION 0xEE12F50F
#define SI_IDENTIFIER_ARL2 0x324C5241


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
    /* Alignment that's used, usually 64. */
    uint32_t alignment;
} siArHeader;
SI_STATIC_ASSERT(sizeof(siArHeader) == 16);


typedef struct {
    /* Entire size of the entry. Equivalent to '.dataSize + .offset = .size' */
    uint32_t size;
    /* Size of the file data itself */
    uint32_t dataSize;
    /* Pointer offset to the data. */
    uint32_t offset;
    /* Always 0 */
    uint32_t unknown1;
    /* Always 0 */
    uint32_t unknown2;
} siArEntry;
SI_STATIC_ASSERT(sizeof(siArEntry) == 20);


typedef struct {
    /* Pointer to the contents of the data. If `siswa_xMake' or 'siswa_xCreateArContent'
     * was used, the data is malloced and must be freed.
    */
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
siArFile siswa_arMakeBuffer(siByte* data, size_t len);
/* Creates a 'siArFile' structure from an archive file's content in memory, while also setting the capacity. Must not be compressed. */
siArFile siswa_arMakeBufferEx(siByte* data, size_t len, size_t capacity);

/* Creates a 'siArFile' structure and allocates an archive file in memory from the provided capacity. */
siArFile siswa_arCreateArContent(size_t capacity);
/* Creates a 'siArFile' structure and creates an archive file in memory from the provided buffer and capacity. */
siArFile siswa_arCreateArContentEx(siByte* buffer, size_t capacity);

siArHeader* siswa_arGetHeader(siArFile arFile); /* TODO */
size_t siswa_arGetEntryCount(siArFile arFile); /* TODO */

/* Polls for the next entry in the archive. It reaches the end once it hits NULL. */
siArEntry* siswa_arEntryPoll(siArFile* arfile);
/* Finds an entry matching the provided name. Returns NULL if the entry doesn't exist. */
siArEntry* siswa_arEntryFind(siArFile arfile, const char* filename);
/* Finds an entry matching the provided name with length. Returns NULL if the entry doesn't exist. */
siArEntry* siswa_arEntryFindEx(siArFile arfile, const char* name, size_t nameLen);

/* Gets the name of the provided entry. */
char* siswa_arEntryGetName(siArEntry* entry);
/* Gets the data of the provided entry. */
siByte* siswa_arEntryGetData(siArEntry* entry);

/* Adds a new entry in the archive. Fails if the entry name already exists or the capacity is too low. */
siResult siswa_arEntryAdd(siArFile* arFile, const char* name, void* data, uint32_t dataSize);
/* Adds a new entry in the archive. Fails if the entry name already exists or the capacity is too low. */
siResult siswa_arEntryAddEx(siArFile* arFile, const char* name, size_t nameLen, void* data, uint32_t dataSize);
/* Removes an entry in the archive. Fails if the entry doesn't exists. */
siResult siswa_arEntryRemove(siArFile* arFile, const char* name);
/* Removes an entry in the archive. Fails if the entry doesn't exists. */
siResult siswa_arEntryRemoveEx(siArFile* arFile, const char* name, size_t nameLen);
/* TODO */
siResult siswa_arEntryUpdate(siArFile* arFile, const char* name, void* data, uint32_t dataSize);

siByte* siswa_arMerge(siArFile mainAr, siArFile secondAr, siByte* outBuffer); /* TODO */
/* Merges multiple archive files into the (copied) main archive file, then
 * returns a buffer of the new archive file. Any duplicating entry names get ignored. */
siByte* siswa_arMergeMul(siArFile mainAr, siArFile* arrayOfArs, size_t arrayLen,
        siByte* outBuffer);

void siswa_arFree(siArFile arFile);

typedef struct {
    /* 'ARL2' at the start of the file. */
    uint32_t identifier;
    /* Total number of archive files. */
    uint32_t archiveCount;
    /* NOTE(EimaMei): File sizes for each file. Length of the array is 'archiveCount'.
       archiveSizes[0] gives the 1st archive file's len, archiveSizes[1] gives
       the 2nd and so forth. */
    uint32_t archiveSizes[SI_UNSPECIFIED_LEN];
} siArlHeader;
SI_STATIC_ASSERT(sizeof(siArlHeader) == 12);

typedef siArFile siArlFile;

typedef struct {
    /* Length of the filename */
    uint8_t len;
    /* The filename The string is NOT null-terminated! */
    char string[SI_UNSPECIFIED_LEN];
} siArlEntry;
SI_STATIC_ASSERT(sizeof(siArlEntry) == 2);


siArlFile siswa_arlMake(const char* path);
siArlFile siswa_arlMakeBuffer(siByte* data, size_t len);

siArlHeader* siswa_arlGetHeader(siArlFile arFile);
size_t siswa_arGetEntryCount(siArlFile arFile); /* TODO */

siArlEntry* siswa_arlEntryPoll(siArlFile* arfile);

siArlFile siswa_arlCreate(siByte* contentBuffer, uint32_t* archiveLengths,
        size_t arrayLen);
#if 0 /* TODO */
siArlEntry* siswa_arlEntryFind(siArFile arfile, const char* filename); /* TODO */
void siswa_arlEntryPollReset(siArFile* arfile); /* TODO */

void siswa_arEntryAdd(siArFile arFile, const char* name, siByte* data, uint32_t dataSize);
void siswa_arEntryRemove(siArFile arFile, const char* name);
void siswa_arEntryUpdate(siArFile arFile, const char* name, siByte* data, uint32_t dataSize);

siByte* siswa_arMerge(siArFile mainAr, siArFile secondAr, siByte* outBuffer); /* TODO */
siByte* siswa_arMergeMul(siArFile mainAr, siArFile* arrayOfArs, size_t arrayLen,
        siByte* outBuffer);
#endif

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

#define SISWA_ARCHIVE_IMPLEMENTATION
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
            return SI_SUCCESS;
        }

increment_entry:
        entry += 1;
        if (entry == end) {
            entry = ht->entries;
        }
    } while (entry != old_entry);

    return SI_FAILURE;
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

    SI_ASSERT_NOT_NULL(path);

    file = fopen(path, "rb");
    SI_ASSERT_NOT_NULL(file);

    fseek(file, 0, SEEK_END);
    dataLen = ftell(file);
	rewind(file);

    data = (siByte*)malloc(dataLen + additionalAllocSpace);
    fread(data, dataLen, 1, file);

    ar = siswa_arMakeBufferEx(data, dataLen, dataLen + additionalAllocSpace);
    fclose(file);

    return ar;
}

siArFile siswa_arMakeBuffer(siByte* data, size_t len) {
    return siswa_arMakeBufferEx(data, len, len);
}
siArFile siswa_arMakeBufferEx(siByte* data, size_t len, size_t capacity) {
    siArFile ar;
    uint32_t identifier;

    SI_ASSERT_NOT_NULL(data);
    SI_ASSERT(len <= capacity, "The length cannot be larger than the total capacity");

    identifier = *(uint32_t*)data;

    if (identifier == SI_IDENTIFIER_XCOMPRESSION) {
        ar.compressed = SI_COMPRESS_XCOMPRESSION;
    }
    else {
        SI_ASSERT(identifier != SI_IDENTIFIER_ARL2, "Use 'siswa_arlFileOpen' for ARL2 files!");
        ar.compressed = SI_COMPRESS_NONE;
    }

    ar.data = data;
    ar.len = len;
    ar.cap = capacity;
    ar.curOffset = sizeof(siArHeader);

    return ar;
}

siArFile siswa_arCreateArContent(size_t capacity) {
    return siswa_arCreateArContentEx(malloc(capacity + sizeof(siArHeader)), capacity);
}

siArFile siswa_arCreateArContentEx(siByte* buffer, size_t capacity) {
    siArHeader header;
    siArFile ar;

    SI_ASSERT(capacity >= sizeof(siArHeader), "Capacity must be at least equal to or be higher than 'sizeof(siArHeader)'.");

    header.unknown = 0;
    header.headerSizeof = sizeof(siArHeader);
    header.entrySizeof = sizeof(siArEntry);
    header.alignment = 64;
    memcpy(buffer, &header, sizeof(siArHeader));

    ar.data = buffer;
    ar.len = sizeof(siArHeader);
    ar.cap = capacity;
    ar.compressed = SI_COMPRESS_NONE;
    ar.curOffset = sizeof(siArHeader);

    return ar;
}

siArEntry* siswa_arEntryPoll(siArFile* arfile) {
    siArEntry* entry = (siArEntry*)(arfile->data + arfile->curOffset);
    if (arfile->curOffset >= arfile->len) {
        arfile->curOffset = sizeof(siArHeader);
        return NULL;
    }

    arfile->curOffset += entry->size;
    return entry;
}

siArEntry* siswa_arEntryFind(siArFile arFile, const char* name) {
    return siswa_arEntryFindEx(arFile, name, strlen(name));
}
siArEntry* siswa_arEntryFindEx(siArFile arfile, const char* name, size_t nameLen) {
    siArEntry* entry;
    arfile.curOffset = sizeof(siArHeader);

    SI_ASSERT_NOT_NULL(name);

    while ((entry = siswa_arEntryPoll(&arfile)) != NULL) {
        char* entryFilename = siswa_arEntryGetName(entry);
        if (*name == *entryFilename && strncmp(name + 1, entryFilename + 1, nameLen - 1) == 0) {
            return entry;
        }
    }

    return NULL;
}

char* siswa_arEntryGetName(siArEntry* entry) {
    return (char*)entry + sizeof(siArEntry);
}
siByte* siswa_arEntryGetData(siArEntry* entry) {
    return (siByte*)entry + entry->offset;
}

siResult siswa_arEntryAdd(siArFile* arFile, const char* name, void* data, uint32_t dataSize) {
    return siswa_arEntryAddEx(arFile, name, strlen(name), data, dataSize);
}
siResult siswa_arEntryAddEx(siArFile* arFile, const char* name, size_t nameLen, void* data, uint32_t dataSize) {
    siArEntry newEntry;
    siByte* dataPtr;
    size_t offset = sizeof(siArHeader);
    {
        siArEntry* entry;
        siArFile tmpArFile = *arFile;

        while ((entry = siswa_arEntryPoll(&tmpArFile)) != NULL) {
            const char* entryName = siswa_arEntryGetName(entry);
            offset = tmpArFile.curOffset;

            if (*name == *entryName && strncmp(name + 1, entryName + 1, nameLen - 1) == 0) {
                return SI_FAILURE;
            }
        }
    }
    newEntry.size = dataSize + nameLen + 1 + sizeof(siArEntry);
    newEntry.dataSize = dataSize;
    newEntry.offset = nameLen + 1 + sizeof(siArEntry);
    newEntry.unknown1 = 0;
    newEntry.unknown2 = 0;

    SI_ASSERT(offset + newEntry.size < arFile->cap, "Not enough space inside the buffer to add a new entry.");
    dataPtr = arFile->data + offset;

    memcpy(dataPtr, &newEntry, sizeof(siArEntry));
    dataPtr += sizeof(siArEntry);

    memcpy(dataPtr, name, nameLen);
    dataPtr += nameLen;
    *dataPtr = '\0';
    dataPtr += 1;

    memcpy(dataPtr, data, dataSize);
    arFile->len += newEntry.size;
    return SI_SUCCESS;
}
siResult siswa_arEntryRemove(siArFile* arFile, const char* name) {
    return siswa_arEntryRemoveEx(arFile, name, strlen(name));
}
siResult siswa_arEntryRemoveEx(siArFile* arFile, const char* name, size_t nameLen) {
    size_t offset;
    siArEntry* entry = siswa_arEntryFindEx(*arFile, name, nameLen);
    siByte* entryPtr = (siByte*)entry;
    if (entry == NULL) {
        return SI_FAILURE;
    }
    offset = (size_t)entry - (size_t)arFile->data;

    arFile->len -= entry->size;
    memmove(entryPtr, entryPtr + entry->size, arFile->len - offset);
    return SI_SUCCESS;
}

siByte* siswa_arMergeMul(siArFile mainAr, siArFile* arrayOfArs, size_t arrayLen,
        siByte* outBuffer) {
    char allocator[SI_DEFAULT_STACK_SIZE];

    siByte* ogBuffer = outBuffer;
    size_t i;
    siHashTable* ht;
    uint32_t res;
    siArFile curAr;
    siArEntry* entry;

    SI_ASSERT_NOT_NULL(arrayOfArs);
    SI_ASSERT_NOT_NULL(outBuffer);

    ht = si_hashtable_make_reserve(allocator, SI_DEFAULT_STACK_SIZE / sizeof(char*) - sizeof(siHashTable));

    {
        siArHeader header;
        header.unknown = 0;
        header.headerSizeof = sizeof(siArHeader);
        header.entrySizeof = sizeof(siArEntry);
        header.alignment = 64; /* TODO(EimaMei): Is the alignment always 64 bytes? */

        memcpy(outBuffer, &header, sizeof(header));
        outBuffer += sizeof(siArHeader);
    }

    for (i = 0; i < arrayLen; i++) {
        curAr = arrayOfArs[i];
        while ((entry = siswa_arEntryPoll(&curAr)) != NULL) {
            char* filename = siswa_arEntryGetName(entry);
            res = si_hashtable_exists(ht, filename);

            if (res == SI_FAILURE) {
                si_hashtable_set(ht, filename);
                memcpy(outBuffer, entry, entry->size);
                outBuffer += entry->size;
            }
        }
    }

    curAr = mainAr;
    while ((entry = siswa_arEntryPoll(&curAr)) != NULL) {
        char* filename = siswa_arEntryGetName(entry);
        res = si_hashtable_exists(ht, filename);
        if (res == SI_FAILURE) {
            memcpy(outBuffer, entry, entry->size);
            outBuffer += entry->size;
        }
    }

    return ogBuffer;
}

void siswa_arFree(siArFile arFile) {
    free(arFile.data);
}

siArlFile siswa_arlMake(const char* path) {
    siArFile ar = siswa_arMake(path);
    return *(siArlFile*)&ar;
}
siArlFile siswa_arlMakeBuffer(siByte* data, size_t len) {
    siArFile ar = siswa_arMakeBuffer(data, len);
    return *(siArlFile*)&ar;
}
siArlHeader* siswa_arlGetHeader(siArlFile arlFile) {
    SI_ASSERT_NOT_NULL(arlFile.data);
    return (siArlHeader*)arlFile.data;
}
siArlEntry* siswa_arlEntryPoll(siArlFile* arlFile) {
    siArlEntry* entry;
    SI_ASSERT_NOT_NULL(arlFile);

    entry = (siArlEntry*)(arlFile->data + arlFile->curOffset);
    if (arlFile->curOffset >= arlFile->len) {
        return NULL;
    }

    arlFile->curOffset += entry->len + 1;
    return entry;
}
void siswa_arlFreeData(siArlFile* arlFile) {
    SI_ASSERT_NOT_NULL(arlFile);
    free(arlFile->data);
}
siArlFile siswa_arlCreate(siByte* contentBuffer, uint32_t* archiveLengths, size_t arrayLen) {
    siArlHeader header;
    size_t offset;
    siArlFile arl;

    SI_ASSERT_NOT_NULL(contentBuffer);
    SI_ASSERT_NOT_NULL(archiveLengths);
    SI_ASSERT(arrayLen != 0, "Archive file count must be higher than 0");

    header.identifier = SI_IDENTIFIER_ARL2;
    header.archiveCount = arrayLen;
    offset = sizeof(siArlHeader) - sizeof(uint32_t);

    memcpy(contentBuffer, &header, offset); /* TODO(EimaMei): Optimize this out somehow. */
    memcpy(contentBuffer + offset, archiveLengths, arrayLen * sizeof(uint32_t));

    arl.data = contentBuffer;
    arl.curOffset = offset + arrayLen * sizeof(uint32_t);
    arl.compressed = SI_COMPRESS_NONE;

    return arl;
}
void siswa_arlFree(siArlFile arFile) {
    free(arFile.data);
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
