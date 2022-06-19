#ifndef __RS_PLATFORM__
#define __RS_PLATFORM__

#include <vector>

#include "nbstd.h"
#include "rsFile.h"

//патхматчспек "*.тхт" функция https://msdn.microsoft.com/en-us/library/windows/desktop/bb773727%28v=vs.85%29.aspx
//MoveFile https://msdn.microsoft.com/en-us/library/windows/desktop/aa365239%28v=vs.85%29.aspx



//-----------------------------------------------------------------------------
enum FILE_TYPE
{
	FT_INVALID,
	FT_DIR,
	FT_FILE
};
FILE_TYPE GetFileType( const std::wstring& realFileName );
inline bool PathExists( const std::wstring& realFileName )
{
	return GetFileType( realFileName ) != FT_INVALID;
}
uniq_ptr<rsFile> ReadFileData( const std::wstring& realFileName );
void ListFiles( const std::wstring& startFolderSlashed, const std::wstring& mask,
				std::vector<uniq_ptr<rsFile>>& realFiles, bool recursive );
void SetFileTime( rsFile& realFile, const NB_DATE newCreate, const NB_DATE newWrite );



//-----------------------------------------------------------------------------
struct WorkingDirectory
{
	WorkingDirectory( const std::wstring& newPath );
	~WorkingDirectory();
	const std::wstring oldPath;
};
std::wstring GetWorkingDirectory();
void SetWorkingDirectory( const std::wstring& dir );



//-----------------------------------------------------------------------------
bool IsPipe();
void GetCmd( std::string& cmd, std::wstring& params );
void SetTitle( const char* message );
void SetTitle( const wchar_t* message );
void ChangeFonts();
void DateToString( const NB_DATE date, std::wstring& str );
void WtoA( const std::wstring& unicode, std::string& ascii );



//-----------------------------------------------------------------------------
inline std::wstring DateToString( const NB_DATE date )
{
	std::wstring str;
	DateToString( date, str );
	return str;
}
inline std::string WtoA( const std::wstring& unicode )
{
	std::string ascii;
	WtoA( unicode, ascii );
	return ascii;
}



#endif __RS_PLATFORM__
