# libSUarchive
A light, fast and portable library for handling Sonic Unleashed's archive file formats (`.ar`/`.arl`). 

# Features
- Ability to go through every archive's entries.
- Request, add, remove or modify any entry you want.
- Get metadata information about the file formats and their entries.
- Merge archive files into one.
- Generate archive linker (`.arl`) files from one or multiple archive files.
- Create your own `.ar`/`.arl` files progrmatically.
- Decompress SEGS (PS3) compressed files into readable .ar/.arl files
- Lightweight as well as single-header, making it easy to implement it in any project.
- Focused on performance so that it wouldn't take forever to do one simple thing, like merging AR files!
- The library is very flexible and can be used in many ways. The library never limits the user to use some hefty dependency, like the C++ STL.
- Written in pure C89, making it portable and work perfectly on most if not all C/C++ compilers.

# Using the library
To use the library, you must do the following in EXACTLY _one_ C/C++ file:
```c
#define SISWA_ARCHIVE_IMPLEMENTATION
#include "libSUarchive.h"
```
Once that's set, other files do not require the '#define' line.

# Planned features
- Add XCompression file support (Limited support for very small files, support for every file will take awhile).
- `.arl` merge functions.
