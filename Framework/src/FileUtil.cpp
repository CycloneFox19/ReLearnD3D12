//--------------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------------
#include "FileUtil.h"

//--------------------------------------------------------------------------------------------------------
//  search for file path
//--------------------------------------------------------------------------------------------------------
bool SearchFilePathA(const char* filename, std::string& result)
{
    if (filename == nullptr)
    {
        return false;
    }

    if (strcmp(filename, " ") == 0 || strcmp(filename, "") == 0)
    {
        return false;
    }

    char exePath[520] = {};
    GetModuleFileNameA(nullptr, exePath, 520);
    exePath[519] = '\0'; // null termination
    PathRemoveFileSpecA(exePath);

    char dstPath[520] = {};

    strcpy_s(dstPath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "..\\%s||", filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "..\\..\\%s", filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "\\res\\%s", filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "%s\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "%s\\..\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "%s\\..\\..\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    sprintf_s(dstPath, "%s\\res\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------------
// search for file path
//--------------------------------------------------------------------------------------------------------
bool SearchFilePathW(const wchar_t* filename, std::wstring& result)
{
	if (filename == nullptr)
    {
        return false;
    }

    if (wcscmp(filename, L" ") == 0 || wcscmp(filename, L"") == 0)
    {
        return false;
    }

    wchar_t exePath[520] = {};
    GetModuleFileNameW(nullptr, exePath, 520);
    exePath[519] = L'\0'; // null termination
    PathRemoveFileSpecW(exePath);

    wchar_t dstPath[520] = {};

    wcscpy_s(dstPath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"..\\%s", filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"..\\..\\%s", filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"\\res\\%s", filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"%s\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"%s\\..\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"%s\\..\\..\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    swprintf_s(dstPath, L"%s\\res\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = dstPath;
        return true;
    }

    return false;
}
