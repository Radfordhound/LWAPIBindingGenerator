static FILE* outputFile = NULL;

static enum CXChildVisitResult visitor(
    CXCursor cursor, CXCursor parent, CXClientData data)
{
    const enum CXCursorKind kind = clang_getCursorKind(cursor);
    switch (kind)
    {
    case CXCursor_AnnotateAttr:
    {
        const auto parentKind = clang_getCursorKind(parent);
        if (clang_isDeclaration(parentKind))
        {
            const char* const addrStr = clang_getCString(
                clang_getCursorSpelling(cursor));

            const char* const mangledName = clang_getCString(
                clang_Cursor_getMangling(parent));

            if (strcmp(addrStr, "TODO") == 0)
            {
                return CXChildVisit_Recurse;
            }

            const unsigned long addr = strtoul(addrStr, NULL, 16);
            fprintf(outputFile, "BINDING %08xh, __imp_%s\n", addr, mangledName);
        }

        return CXChildVisit_Recurse;
    }

    default:
        return CXChildVisit_Recurse;
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

#ifdef _WIN32
    outputFile = _wfopen(argv[2], L"w");
#else
    outputFile = fopen(argv[2], "w");
#endif

    fputs(".MODEL FLAT\n\n", outputFile);
    fputs("include LWAPI.inc\n\n", outputFile);

    CXIndex idx = clang_createIndex(true, true);

    const char* const libClangArgs[] =
    {
        "-xc++",
        "-m32",
        "-D",
        "LWAPI(addrWiiU, addrPC)=__attribute__((annotate(#addrPC)))"
    };

    static const int libClangArgsCount =
        (sizeof(libClangArgs) / sizeof(*libClangArgs));

    CXTranslationUnit tu = clang_createTranslationUnitFromSourceFile(
        idx, inputFilePath, libClangArgsCount, libClangArgs, 0, NULL);

    clang_visitChildren(clang_getTranslationUnitCursor(tu), visitor, NULL);

    fputs("\nEND\n", outputFile);
    fclose(outputFile);

    return EXIT_SUCCESS;
}
