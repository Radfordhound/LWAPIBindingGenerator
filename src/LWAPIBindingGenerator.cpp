struct FileDeleter
{
    inline void operator()(FILE* f)
    {
        std::fclose(f);
    }
};

using FileSmartPointer = std::unique_ptr<std::FILE, FileDeleter>;

static FileSmartPointer outputFile = nullptr;

static CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData data)
{
    const auto kind = clang_getCursorKind(cursor);
    switch (kind)
    {
    case CXCursor_AnnotateAttr:
    {
        const auto parentKind = clang_getCursorKind(parent);
        if (clang_isDeclaration(parentKind))
        {
            const auto addrStr = clang_getCString(clang_getCursorSpelling(cursor));
            const auto mangledName = clang_getCString(clang_Cursor_getMangling(parent));

            if (std::strcmp(addrStr, "TODO") == 0)
            {
                return CXChildVisit_Recurse;
            }

            const auto addr = std::stoul(addrStr, nullptr, 16);
            std::fprintf(outputFile.get(), "BINDING %08x, %s\n", addr, mangledName);
        }

        return CXChildVisit_Recurse;
    }

    default:
        return CXChildVisit_Recurse;
    }
}

std::string ToUTF8Str(const wchar_t* str)
{
    const auto len = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, NULL, NULL);
    if (!len) throw std::runtime_error("Could not convert UTF-16 string to UTF-8");

    std::string result(len, '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, str, -1, &result[0], len, NULL, NULL))
    {
        throw std::runtime_error("Could not convert UTF-16 string to UTF-8");
    }

    return result;
}

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
#ifdef _WIN32
    outputFile = FileSmartPointer(_wfopen(argv[2], L"w"));
#else
    outputFile = FileSmartPointer(std::fopen(argv[2], "w"));
#endif

    std::fputs(".MODEL FLAT\n", outputFile.get());
    std::fputs("include \"LWAPI.inc\"\n", outputFile.get());

    CXIndex idx = clang_createIndex(true, true);

    const char* const libClangArgs[] =
    {
        "-xc++",
        "-m32",
        "-D",
        "LWAPI(addrWiiU, addrPC)=__declspec(dllimport) __attribute__((annotate(#addrPC)))"
    };

    constexpr static auto libClangArgsCount = (sizeof(libClangArgs) / sizeof(*libClangArgs));

#ifdef _WIN32
    std::string inputFilePathBuf = ToUTF8Str(argv[1]);
    const char* inputFilePath = inputFilePathBuf.c_str();
#else
    const char* inputFilePath = argv[1];
#endif

    CXTranslationUnit tu = clang_createTranslationUnitFromSourceFile(
        idx, inputFilePath, libClangArgsCount, libClangArgs, 0, nullptr);

    clang_visitChildren(clang_getTranslationUnitCursor(tu), visitor, nullptr);

    std::fputs("\nEND", outputFile.get());
    return EXIT_SUCCESS;
}
