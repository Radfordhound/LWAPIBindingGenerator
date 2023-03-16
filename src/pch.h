#pragma once

// Windows
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

// Standard library
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// libClang
#include <clang-c/Index.h>
#include <clang-c/CXString.h>
