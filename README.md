# libSUarchive
A light, fast and portable library for handling Sonic Unleashed's archive file format

# Features
- Ability to go through every archive's entry.
- Add, remove or modify any entry you request.
- Get metadata information about the file formats and their entries.
- Merge archive files into one.
- Lightweight as well as single-header, making it easy to implement it in any project.
- Focused on performance so that it wouldn't take forever to do one simple thing, like merging AR files!
- The library is very flexible and can be used in many ways. The library never limits the user to use some hefty dependency, like the C++ STL.
- Written in pure C89, making it portable and work perfectly on most if not all C or C++ compilers.

# Using the library
The first time you include the library, you must define `SISWA_ARCHIVE_IMPLEMENTATION` before it so that the source could get compiled. 
```c
#define SISWA_ARCHIVE_IMPLEMENTATION
#include "libSUarchive.h"
```
Any other include afterwards doesn't require the define.
