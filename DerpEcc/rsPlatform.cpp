#include "rsPlatform.h"

#include <Windows.h>

#include "nbstd.h"
#include "rsUtils.h"

//In the ANSI version of this function, the name is limited to MAX_PATH characters. 
//To extend this limit to approximately 32,000 wide characters, call the Unicode 
//version of the function (FindFirstFileExW), and prepend "\\?\" to the path. 
//For more information, see Naming a File.



//-----------------------------------------------------------------------------
namespace nb {
namespace enforcer {
struct WinApiRaiser
{
	template <class T>
	static void Throw( const T&, const std::string& message, const char* locus )
	{
		char wmsg[1024];
		char wmid[33];
		DWORD err = GetLastError();
		_itoa_s( err, wmid, 33, 10 );

		//If the function fails, the return value is zero.
		//MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), wmsg, 1024, NULL );

		throw std::runtime_error( message + "\nWinError " + wmid + ": " + wmsg + locus );
	}

	#define ENFORCE_WAPI(exp) *nb::enforcer::MakeEnforcer< \
		nb::enforcer::DefaultPredicate, nb::enforcer::WinApiRaiser>( (exp), \
		__FILE__ "\\" NB_MACRO_STRINGIZE(__LINE__) ": '" #exp "'" )
};
} //enforcer
} //nb



//-----------------------------------------------------------------------------
template<BOOL WINAPI HandleCloser(HANDLE)>
struct RAII_HANDLE
{
	RAII_HANDLE()
		: mHandle( INVALID_HANDLE_VALUE ) {
	}
	RAII_HANDLE( HANDLE handle )
		: mHandle( handle ) {
	}
	~RAII_HANDLE() {
		reset( INVALID_HANDLE_VALUE );
	}
	void reset( HANDLE handle ) {
		if( mHandle != INVALID_HANDLE_VALUE )
			ENFORCE_WAPI( HandleCloser(mHandle) )
				("faulty handle: '")(mHandle)("'");
		mHandle = handle;
	}
	RAII_HANDLE& operator = ( const HANDLE handle ) {
		reset( handle );
		return *this;
	}
	operator HANDLE () {
		return mHandle;
	}
private:
	HANDLE mHandle;
};
typedef RAII_HANDLE<CloseHandle> FileHandle;
typedef RAII_HANDLE<FindClose> SearchHandle;



//-----------------------------------------------------------------------------
static inline void operator << ( NB_DATE& left, const FILETIME& right ) {
	left.hi = right.dwHighDateTime;
	left.lo = right.dwLowDateTime;
}
static inline void operator << ( FILETIME& left, const NB_DATE& right ) {
	left.dwHighDateTime = right.hi;
	left.dwLowDateTime = right.lo;
}
static inline int64 HiLo_i64( DWORD hi, DWORD lo ) {
	return (int64(hi) << 32) | lo;
}



//-----------------------------------------------------------------------------
void WtoA( const std::wstring& unicode, std::string& ascii )
{
	ascii.resize( unicode.size() );
	if( unicode.empty() ) return;

	BOOL incompochar = FALSE;
	int size = size_t_u32( unicode.size() );
	//int lenght = 
	ENFORCE_WAPI( WideCharToMultiByte(CP_ACP/*1251*/, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
		unicode.c_str(), size, &ascii[0], size, NULL, &incompochar) )
		("wide string to narrow conversion failed, and I can't even print faulty string because of that");
	//ascii.resize( lenght );
}



//-----------------------------------------------------------------------------
uniq_ptr<rsFile> ReadFileData( const std::wstring& realFileName )
{
	/*FILETIME ftCreate, ftWrite;
	LARGE_INTEGER lpFileSize;
	{
		FileHandle hFile = CreateFileW( realFileName.c_str(),
			GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
		ENFORCE_WAPI( hFile != INVALID_HANDLE_VALUE )
			("opening windows file handle failed on '")(realFileName)("'");
		ENFORCE_WAPI( GetFileTime(hFile, &ftCreate, 0, &ftWrite) )
			("GetFileTime failed on '")(realFileName)("'");
		ENFORCE_WAPI( GetFileSizeEx(hFile, &lpFileSize) )
			("GetFileSizeEx failed on '")(realFileName)("'");
	}*/
	//гет файл атрибьютс из бетер зан опенфайл:
	//https://msdn.microsoft.com/en-us/library/windows/desktop/aa364946%28v=vs.85%29.aspx
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;

	ENFORCE_WAPI( GetFileAttributesExW(realFileName.c_str(),
		GetFileExInfoStandard, &fileInfo) )
		("GetFileAttributesEx failed on '")(realFileName)("'");

	uniq_ptr<rsFile> realFile = new rsFile();
	realFile->name = realFileName;
	realFile->writeDate << fileInfo.ftLastWriteTime;
	realFile->createDate << fileInfo.ftCreationTime;

	/*LARGE_INTEGER lpFileSize;
	lpFileSize.HighPart = fileInfo.nFileSizeHigh;
	lpFileSize.LowPart = fileInfo.nFileSizeLow;
	realFile->fileSize = lpFileSize.QuadPart;*/
	realFile->fileSize = HiLo_i64( fileInfo.nFileSizeHigh, fileInfo.nFileSizeLow );

	/*{
		realFile->name = realFileName;

		realFile->writeDate.hi = ftWrite.dwHighDateTime;
		realFile->writeDate.lo = ftWrite.dwLowDateTime;

		realFile->createDate.hi = ftCreate.dwHighDateTime;
		realFile->createDate.lo = ftCreate.dwLowDateTime;

		realFile->fileSize = lpFileSize.QuadPart;
	}*/

	return realFile;
}



//-----------------------------------------------------------------------------
void SetFileTime( rsFile& realFile, const NB_DATE newCreate, const NB_DATE newWrite )
{
	FileHandle hFile = CreateFileW( realFile.name.c_str(),
		GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
	ENFORCE_WAPI( hFile != INVALID_HANDLE_VALUE )
		("opening windows file handle failed on '")(realFile.name)("'");

	FILETIME ftCreate, ftWrite;
	ftCreate << newCreate;
	ftWrite << newWrite;

	ENFORCE_WAPI( SetFileTime(hFile, &ftCreate, 0, &ftWrite) )
		("SetFileTime failed on '")(realFile.name)("'");

	realFile.createDate = newCreate;
	realFile.writeDate = newWrite;
}



//-----------------------------------------------------------------------------
static void CopyParams( const WIN32_FIND_DATAW& ffd, rsFile& file )
{
	file.writeDate << ffd.ftLastWriteTime;
	file.createDate << ffd.ftCreationTime;

	file.fileSize = int64(ffd.nFileSizeHigh) << 32;
	file.fileSize += ffd.nFileSizeLow;
}



//-----------------------------------------------------------------------------
static void FindFiles( const std::wstring& startFolderSlashed, const std::wstring& mask,
						std::vector<uniq_ptr<rsFile>>& realFiles, bool recursive )
{
	static const std::wstring thisFolder = L".";
	static const std::wstring parentFolder = L"..";
	const std::wstring searchString = startFolderSlashed + mask;

	WIN32_FIND_DATAW ffd;
	SearchHandle hSearch = FindFirstFileW( searchString.c_str(), &ffd );
//*/FindFirstFileExW( searchString.c_str(), FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, NULL );
	if( hSearch == INVALID_HANDLE_VALUE ) {
		ENFORCE_WAPI( GetLastError() == ERROR_FILE_NOT_FOUND )
			("FindFirstFile failed on '")(searchString)("'");
		return;
	}

	do {
		if( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if( !recursive ) continue;
			if( ffd.cFileName == thisFolder ) continue;
			if( ffd.cFileName == parentFolder ) continue;

			const std::wstring subfolder = startFolderSlashed + ffd.cFileName + L"\\";
			FindFiles( subfolder, mask, realFiles, recursive );
		} else {
			uniq_ptr<rsFile> realFile = new rsFile();
			CopyParams( ffd, *realFile );
			realFile->name = startFolderSlashed + ffd.cFileName;
			realFiles.push_back( realFile );
		}
	}
	while( FindNextFileW(hSearch, &ffd) );

	ENFORCE_WAPI( GetLastError() == ERROR_NO_MORE_FILES )
		("FindNextFile failed on '")(searchString)("'");
}



//-----------------------------------------------------------------------------
static void FindFoldersRecursive( const std::wstring& startFolderSlashed, std::vector<std::wstring>& folders )
{
	static const std::wstring thisFolder = L".";
	static const std::wstring parentFolder = L"..";
	const std::wstring searchString = startFolderSlashed + L"*";

	WIN32_FIND_DATAW ffd;
	SearchHandle hSearch = FindFirstFileW( searchString.c_str(), &ffd );
	if( hSearch == INVALID_HANDLE_VALUE ) {
		ENFORCE_WAPI( GetLastError() == ERROR_FILE_NOT_FOUND )
			("FindFirstFile failed on '")(searchString)("'");
		return;
	}

	do {
		if( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if( ffd.cFileName == thisFolder ) continue;
			if( ffd.cFileName == parentFolder ) continue;

			const std::wstring subfolder = startFolderSlashed + ffd.cFileName + L"\\";
			folders.push_back( subfolder );
			FindFoldersRecursive( subfolder, folders );
		}
	}
	while( FindNextFileW(hSearch, &ffd) );

	ENFORCE_WAPI( GetLastError() == ERROR_NO_MORE_FILES )
		("FindNextFile failed on '")(searchString)("'");
}



//-----------------------------------------------------------------------------
void ListFiles( const std::wstring& startFolderSlashed, const std::wstring& mask, 
				std::vector<uniq_ptr<rsFile>>& realFiles, bool recursive )
{
	std::vector<std::wstring> folders;
	folders.push_back( startFolderSlashed );

	if( recursive )
		FindFoldersRecursive( startFolderSlashed, folders );

	for( std::size_t i = 0, e = folders.size(); i < e; i++ )
		FindFiles( folders[i], mask, realFiles, false );
}



//-----------------------------------------------------------------------------
FILE_TYPE GetFileType( const std::wstring& realFileName )
{
	DWORD fileAttributes = GetFileAttributesW( realFileName.c_str() );
	//If the function fails, the return value is INVALID_FILE_ATTRIBUTES.
	//To get extended error information, call GetLastError.

	if( fileAttributes == INVALID_FILE_ATTRIBUTES )
		return FT_INVALID;

	if( fileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		return FT_DIR;
	else
		return FT_FILE;
}



//-----------------------------------------------------------------------------
static void BrushPathSlashes( std::wstring& path )
{
	for( std::size_t i = 0, e = path.size(); i < e; ++i )
		if( path[i] == '/' ) path[i] = '\\';
}



//-----------------------------------------------------------------------------
// ReadConsoleW crashes on read from pipe, so we can't use it there
// but, at the same time, it handles way more characters than wcin
// so we fallback on wcin for pipe, but use full charset on normal launch
#include <stdio.h> //_fileno, stdin
#include <io.h> //_isatty
bool IsPipe()
{
	return _isatty( _fileno(stdin) ) == 0;
}



//-----------------------------------------------------------------------------
void GetCmd( std::string& cmd, std::wstring& params )
{
	std::wstring input( 300, 0 );
	if( IsPipe() )
	{
		std::getline( std::wcin, input );
	}
	else
	{
		DWORD numread = 0;
		HANDLE console = GetStdHandle( STD_INPUT_HANDLE );
		ENFORCE_WAPI( console != INVALID_HANDLE_VALUE )("GetStdHandle failed");
		ENFORCE_WAPI( ReadConsoleW(console, &input[0], size_t_u32(input.size()), &numread, NULL) )
			("failed to read console input");

		const std::size_t end = input.find_last_not_of( L"\r\n", numread - 1 );
		input.resize( end + 1 ); //lol, pri npos + 1 it overflows to zero and produces desired result
	}

	const std::size_t split = input.find_first_of(' ');
	WtoA( input.substr( 0, split ), cmd );

	if( split != std::string::npos ) {
		params = input.substr( split + 1 );
		BrushPathSlashes( params );
	} else {
		params.clear();
	}
}



//-----------------------------------------------------------------------------
void SetTitle( const char* message )
{
	ENFORCE_WAPI( SetConsoleTitleA(message) )
		("can't SetTitle to '")(message)("'");
}



//-----------------------------------------------------------------------------
void SetTitle( const wchar_t* message )
{
	ENFORCE_WAPI( SetConsoleTitleW(message) )
		("can't SetTitle to '")(message)("'");
}



//-----------------------------------------------------------------------------
std::wstring GetWorkingDirectory()
{
	DWORD buffSize = ENFORCE_WAPI( GetCurrentDirectoryW(0, 0) )
		("GetWorkingDirectory can't estimate buffer size");

	std::wstring str( buffSize, '\0' );
	DWORD strLen = ENFORCE_WAPI( GetCurrentDirectoryW(size_t_u32(str.size()), &str[0]) )
		("GetWorkingDirectory failed to fill the buffer");

	ASSERT( buffSize > strLen )("wat");
	str.resize( strLen );
	return str;
}



//-----------------------------------------------------------------------------
void SetWorkingDirectory( const std::wstring& dir )
{
	if( !dir.empty() )
		ENFORCE_WAPI( SetCurrentDirectoryW(dir.c_str()) )
			("can't SetWorkingDirectory to '")(dir)("'");
}



//-----------------------------------------------------------------------------
WorkingDirectory::WorkingDirectory( const std::wstring& newPath )
	: oldPath( GetWorkingDirectory() )
{
	if( newPath.size() && newPath != oldPath + L"\\" ) {
		SetWorkingDirectory( newPath );
	/*	ENFORCE( (GetWorkingDirectory() != oldPath) || (GetWorkingDirectory() == newPath) )
			("can't change working directory! newPath = '")(newPath)
			("', GetWorkingDirectory() = '")(GetWorkingDirectory())("'"); */
	}
}



//-----------------------------------------------------------------------------
WorkingDirectory::~WorkingDirectory()
{
	SetWorkingDirectory( oldPath );
}



//-----------------------------------------------------------------------------
void ChangeFonts()
{
	// Sets codepage of text input to be russian-recognizable,
	// Does not affect the output //does, after setting, in Lucinda font
	ENFORCE_WAPI( SetConsoleOutputCP(1251) );
	ENFORCE_WAPI( SetConsoleCP(1251) );
}



//-----------------------------------------------------------------------------
void DateToString( const NB_DATE date, std::wstring& str )
{
	SYSTEMTIME sysTime;
	FILETIME fileTime;
	fileTime << date;

	ENFORCE_WAPI( FileTimeToLocalFileTime(&fileTime, &fileTime) )
		("FileTimeToLocalFileTime failed");

	ENFORCE_WAPI( FileTimeToSystemTime(&fileTime, &sysTime) )
		("FileTimeToSystemTime failed");

	const int buffSize = 32;
	wchar_t buffDate[buffSize];
	wchar_t buffTime[buffSize];

	//DATE_SHORTDATE
	ENFORCE_WAPI( GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE,
		&sysTime, NULL, buffDate, buffSize) )
		("GetDateFormat failed");

	ENFORCE_WAPI( GetTimeFormatW(LOCALE_USER_DEFAULT, 0,
		&sysTime, NULL, buffTime, buffSize) )
		("GetTimeFormat failed");

	str  = buffDate;
	str += ' ';
	str += buffTime;
}
