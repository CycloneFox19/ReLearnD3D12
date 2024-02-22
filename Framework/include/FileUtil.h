#pragma once

//--------------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------------
#include <string>
#include <Shlwapi.h>

//--------------------------------------------------------------------------------------------------------
// Linker
//--------------------------------------------------------------------------------------------------------
#pragma comment(lib, "shlwapi.lib")


//--------------------------------------------------------------------------------------------------------
//! @brief search for file path
//! 
//! @param[in] filename name of the file to search
//! @param[out] result container of seeking result
//! @retval true file was found
//! @retval false file wasn't found
//! @memo the following is searching priority
//!		.\
//!		..\
//!		..\..\
//!		.\res\
//!		%EXE_DIR%\
//!		%EXE_DIR%\..\
//!		%EXE_DIR%\..\..\
//!		%EXE_DIR%\res\
//--------------------------------------------------------------------------------------------------------
bool SearchFilePathA(const char* filename, std::string& result);

//--------------------------------------------------------------------------------------------------------
//! @brief search for file path
//! 
//! @param[in] filename name of the file to search
//! @param[out] result container of seeking result
//! @retval true file was found
//! @retval false file wasn't found
//! @memo the following is searching priority
//!		.\
//!		..\
//!		..\..\
//!		.\res\
//!		%EXE_DIR%\
//!		%EXE_DIR%\..\
//!		%EXE_DIR%\..\..\
//!		%EXE_DIR%\res\
//--------------------------------------------------------------------------------------------------------
bool SearchFilePathW(const wchar_t* filename, std::wstring& result);

#if defined(UNICODE) || defined(_UNICODE)
inline bool SearchFilePath(const wchar_t* filename, std::wstring& result)
{
	return SearchFilePathW(filename, result);
}
#else
inline bool SearchFilePath(const char* filename, std::string& result)
{
	return SearchFilePathA(filename, result);
}
#endif // defined(UNICODE) || defined(_UNICODE)