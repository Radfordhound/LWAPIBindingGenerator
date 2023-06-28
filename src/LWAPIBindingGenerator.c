static enum CXChildVisitResult visitor(
    CXCursor cursor, CXCursor parent, CXClientData data)
{
    // Parse the current cursor based on its kind.
    FILE* const outputFile = (FILE*)data;
    const enum CXCursorKind kind = clang_getCursorKind(cursor);

    switch (kind)
    {
    case CXCursor_AnnotateAttr:
    {
        const enum CXCursorKind parentKind = clang_getCursorKind(parent);
        if (clang_isDeclaration(parentKind))
        {
            // Get address string from LWAPI macro.
            const char* const addrStr = clang_getCString(
                clang_getCursorSpelling(cursor));

            // Skip if this string is "TODO" or "NONE".
            if (strcmp(addrStr, "TODO") == 0 || strcmp(addrStr, "NONE") == 0)
            {
                return CXChildVisit_Recurse;
            }

            // Otherwise, assume the string is parsable to an unsigned long, and parse it as such.
            const unsigned long addr = strtoul(addrStr, NULL, 16);

            // Get a list of all mangled names for the parent declaration.
            const CXStringSet* const mangledNames = clang_Cursor_getCXXManglings(parent);
            if (mangledNames)
            {
                // If there are multiple mangled names, generate a binding for each one.
                for (unsigned int i = 0; i < mangledNames->Count; ++i)
                {
                    fprintf(outputFile, "BINDING %08xh, __imp_%s\n", addr,
                        clang_getCString(mangledNames->Strings[i]));
                }
            }
            else
            {
                // If there is only one mangled name, generate a binding just for it.
                // NOTE: For some reason, some symbols with only one mangling return
                // a value from clang_Cursor_getCXXManglings, while others do not.
                // Thus why we need to handle both cases.
                const char* const mangledName = clang_getCString(
                    clang_Cursor_getMangling(parent));

                fprintf(outputFile, "BINDING %08xh, __imp_%s\n", addr, mangledName);
            }
        }

        return CXChildVisit_Recurse;
    }

    default:
        return (clang_Location_isFromMainFile(clang_getCursorLocation(cursor))) ?
            CXChildVisit_Recurse : CXChildVisit_Continue;
    }
}

#ifdef _WIN32
int ToUTF8Str(const wchar_t* str, char* result, int resultBufLen)
{
    return WideCharToMultiByte(CP_UTF8, 0, str, -1,
        result, resultBufLen, NULL, NULL);
}

int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    // Get input file path.
#ifdef _WIN32
    char inputFilePath[MAX_PATH];
    if (!ToUTF8Str(argv[1], inputFilePath, MAX_PATH))
    {
        return EXIT_FAILURE;
    }
#else
    const char* inputFilePath = argv[1];
#endif

    // Get vs2010 install directory.
#ifdef _WIN32
    const char* const win32Vs2010InstallDir = getenv("VS100COMNTOOLS");
    char win32Vs2010IncludeDir[MAX_PATH];

    sprintf(win32Vs2010IncludeDir, "%s..\\..\\VC\\include", win32Vs2010InstallDir);
#endif

    // Generate libClang arguments.
    const char* const builtInArgs[] =
    {
        "-xc++",
        "-m32",
        "-fms-compatibility",
        "-fmsc-version=1600",
        //"-fms-compatibility-version=10.0.30319.1",
        //"-fms-compatibility-version=1600",
        "-nostdinc",
        "-nostdlib",
        //"-nostdinc++",
        //"-nostdlib++",
        //"-Wno-narrowing",
#ifdef _WIN32
        "-isystem",
        win32Vs2010IncludeDir,
        "-isystem",
        "C:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.0A\\Include", // TODO: Avoid hardcoding this somehow?
        "-DBOOST_USE_WINDOWS_H",
#endif
        "-DLWAPI(addrWiiU, addrPC)=__attribute__((annotate(#addrPC)))",
        "-DOPENLW_PRIVATE=public:",
        "-DOPENLW_PROTECTED=public:",
        "-DOPENLW_STD_NAMESPACE=::std::",
        "-DLWAPI_STATIC_ASSERT_SIZE(type, size)=static_assert(sizeof(type) == (size),"
            "\"sizeof(\" #type \") != expected size (\" #size \")\");",

        //"-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH", // for msvc stl
    };

    static const int builtInArgsCount =
        (sizeof(builtInArgs) / sizeof(*builtInArgs));

    const int includeDirCount = (argc - 3);
    const int libClangArgsCount = (builtInArgsCount + (includeDirCount
#ifndef _WIN32
        // Allocate one extra argument per include directory on non-Windows,
        // so we can do "--include-directory" followed by argv[x] and avoid
        // allocating copies of each of the include directories.
        * 2
#endif
    ));

#ifdef _WIN32
    // On Windows, we have to convert arguments to UTF-8.
    int win32TotalIncludeDirLen = 0;
    for (int i = 3; i < argc; ++i)
    {
        // Determine how many chars we'll need to convert this argument to UTF-8.
        const int win32CurIncludeDirLen = ToUTF8Str(argv[i], NULL, 0);
        if (!win32CurIncludeDirLen) return EXIT_FAILURE;

        // Add this number to the total length.
        win32TotalIncludeDirLen += 2; // For "-I".
        win32TotalIncludeDirLen += win32CurIncludeDirLen;
    }
#endif

    const char** const libClangArgs = malloc(
        (sizeof(const char** const) * libClangArgsCount)
#ifdef _WIN32
        // Allocate extra space on Windows for UTF-8 argument conversion.
        + win32TotalIncludeDirLen
#endif
        );
    
    // Copy built-in arguments.
    memcpy(libClangArgs, builtInArgs, sizeof(builtInArgs));

    // Copy user-provided include directories.
    const char** curIncludeDirPtr = (libClangArgs + builtInArgsCount);

#ifdef _WIN32
    char* win32CurIncludeDir = (char*)(libClangArgs + libClangArgsCount);
#endif

    for (int i = 3; i < argc; ++i)
    {
#ifdef _WIN32
        // Convert current include directory to UTF-8 and store result in buffer.
        *(curIncludeDirPtr++) = win32CurIncludeDir;

        win32CurIncludeDir[0] = '-';
        win32CurIncludeDir[1] = 'I';
        win32CurIncludeDir += 2;

        const int win32CurIncludeDirLen = ToUTF8Str(
            argv[i], win32CurIncludeDir, win32TotalIncludeDirLen);

        if (!win32CurIncludeDirLen)
        {
            free(libClangArgs);
            return EXIT_FAILURE;
        }

        win32TotalIncludeDirLen -= win32CurIncludeDirLen;
        win32CurIncludeDir += win32CurIncludeDirLen;
#else
        *(curIncludeDirPtr++) = "--include-directory";
        *(curIncludeDirPtr++) = argv[i];
#endif
    }

    // Read input header file and parse it into a libClang translation unit.
    CXIndex idx = clang_createIndex(true, true);

    CXTranslationUnit tu = clang_createTranslationUnitFromSourceFile(
        idx, inputFilePath, libClangArgsCount, libClangArgs, 0, NULL);

    // Free libClang arguments buffer.
    free(libClangArgs);

    // Open output file.
#ifdef _WIN32
    FILE* const outputFile = _wfopen(argv[2], L"w");
#else
    FILE* const outputFile = fopen(argv[2], "w");
#endif

    if (!outputFile) return EXIT_FAILURE;

    // Write "header".
    fputs(".MODEL FLAT\n\n", outputFile);
    fputs("include LWAPI.inc\n\n", outputFile);

    // Visit children in input header file, writing to file in the process.
    clang_visitChildren(clang_getTranslationUnitCursor(tu), visitor, outputFile);

    // Write "footer".
    fputs("\nEND\n", outputFile);

    // Close file and return.
    fclose(outputFile);
    return EXIT_SUCCESS;
}
