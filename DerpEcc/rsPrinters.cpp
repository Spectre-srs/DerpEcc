#include <ctime>

#include "rsPrinters.h"
#include "rsPlatform.h"
#include "rsFileComparison.h"
#include "rsUtils.h"



//-----------------------------------------------------------------------------
std::string FileSizeToString( uint64 bytes )
{
	const char* sizes[] = { "B", "KB", "MB", "GB", "TB" };
	const float step = 1024;
	int order = 0;
	char buff[16];

	if( bytes < step )
	{
		sprintf_s( buff, sizeof(buff), "%llu %s", bytes, sizes[order] );
	}
	else
	{
		float size = (float)bytes;
		do
		{
			size /= step;
			++order;
		}
		while( size > step ); //&& order < arrsize(sizes)
		sprintf_s( buff, sizeof(buff), "%.2f %s", size, sizes[order] );
	}

	return buff;
}
static std::string TimeToString( clock_t time )
{
	const char* sizes[] = { "s", "m", "h" };
	const clock_t step = 60;
	int order = 0;
	char buff[16];

	time /= CLOCKS_PER_SEC;
	if( time > 99 )
	{
		time /= step;
		++order;
	}

	sprintf_s( buff, sizeof(buff), "%ld%s", time, sizes[order] );
	return buff;
}
static std::string ToString( uint64 num )
{
	char buff[32];
	sprintf_s( buff, sizeof(buff), "%llu", num );
	return buff;
}



//-----------------------------------------------------------------------------
static std::string ProgressBar( uint64 from, uint64 to )
{
	const std::size_t width = 19;
	const std::size_t amount = (std::size_t)(from / float(to) * (width+1));

	std::string str( amount, 'I' );
	str.resize( width, '!' );
	return str;
}
static std::string ProgressCount( uint64 from, uint64 to )
{
	char from_s[33];
	sprintf_s( from_s, sizeof(from_s), "%llu", from );
	char to_s[33];
	sprintf_s( to_s, sizeof(to_s), "%llu", to );

	std::string str = from_s;
	str += "/";
	str += to_s;
	return str;
}
static std::string ProgressMass( uint64 from, uint64 to )
{
	std::string str = FileSizeToString( from );
	str += "/";
	str += FileSizeToString( to );
	return str;
}



//-----------------------------------------------------------------------------
static std::size_t gs_CounterCountCurrent = 0;
static std::size_t gs_CounterCountTarget = 0;
static uint64 gs_CounterMassCurrent = 0;
static uint64 gs_CounterMassTarget = 0;
static std::string gs_CounterMessage;
static clock_t gs_CounterUpdateNext = clock();
static clock_t gs_CounterTimeStarted = clock();
static const clock_t gsc_CounterUpdateFrequency = CLOCKS_PER_SEC / 10;
static const clock_t gsc_CounterDisplayTimeDelay = CLOCKS_PER_SEC * 2;
static const clock_t gsc_CounterMinTimeToDisplay = CLOCKS_PER_SEC * 5;
static void CounterPrint()
{
	const bool countCounts = gs_CounterCountTarget > 0;
	const bool massCounts = gs_CounterMassTarget > 0;
	const clock_t currentTime = clock();
	ENFORCE( countCounts || massCounts )("printing with both targets zero");
	if( gs_CounterMassCurrent > gs_CounterMassTarget )
		std::cout << "!mass counter overflow  ";
	if( gs_CounterCountCurrent > gs_CounterCountTarget )
		std::cout << "!count counter overflow  ";

	if( currentTime < gs_CounterUpdateNext ) {
		if( massCounts ) {
			if( gs_CounterMassCurrent != gs_CounterMassTarget ) return;
		}
		else {
			if( gs_CounterCountCurrent != gs_CounterCountTarget ) return;
		}
	} else {
		gs_CounterUpdateNext = currentTime + gsc_CounterUpdateFrequency;
	}

	std::string status;
	if( massCounts ) {
		status = ProgressBar( gs_CounterMassCurrent, gs_CounterMassTarget );
	} else {
		status = ProgressBar( gs_CounterCountCurrent, gs_CounterCountTarget );
	}
	status += "  - ";

	if( countCounts )
	{
		status += ProgressCount( gs_CounterCountCurrent, gs_CounterCountTarget );
		if( massCounts ) {
			status += "  (";
			status += ProgressMass( gs_CounterMassCurrent, gs_CounterMassTarget );
			status += ")";
		}
	} else {
		status += ProgressMass( gs_CounterMassCurrent, gs_CounterMassTarget );
	}

	{
		float progress = 0;
		if( massCounts )
			progress = gs_CounterMassCurrent / (float)gs_CounterMassTarget;
		else if( countCounts )
			progress = gs_CounterCountCurrent / (float)gs_CounterCountTarget;

		if( progress != 0 )
		{
			const clock_t elapsedTime = currentTime - gs_CounterTimeStarted;
			const clock_t estimatedTime = static_cast<clock_t>(elapsedTime / progress);
			const clock_t timeToComplete = estimatedTime - elapsedTime;
			if( elapsedTime > gsc_CounterDisplayTimeDelay && estimatedTime > gsc_CounterMinTimeToDisplay )
			{
				status += " ";
				status += TimeToString( timeToComplete );
			}
		}
	}

	status += gs_CounterMessage;
	SetTitle( status.c_str() );
}
void CounterClear()
{
	gs_CounterCountCurrent = gs_CounterCountTarget = 0;
	gs_CounterMassCurrent = gs_CounterMassTarget = 0;
	gs_CounterMessage.clear();
}
void CounterSetTargetCount( std::size_t count, const char* msg )
{
	CounterSetTarget( count, 0, msg );
}
void CounterSetTargetMass( uint64 mass, const char* msg )
{
	CounterSetTarget( 0, mass, msg );
}
void CounterSetTarget( std::size_t count, uint64 mass, const char* msg )
{
	CounterClear();
	if( !count && !mass ) return;
	gs_CounterTimeStarted = clock();
	gs_CounterCountTarget = count;
	gs_CounterMassTarget = mass;
	gs_CounterMessage = msg;
	CounterPrint();
}
void CounterAdvanceCount( std::size_t count )
{
	CounterAdvance( count, 0 );
}
void CounterAdvanceMass( uint64 mass )
{
	CounterAdvance( 0, mass );
}
void CounterAdvance( std::size_t count, uint64 mass )
{
	if( !count && !mass ) return;
	gs_CounterCountCurrent += count;
	gs_CounterMassCurrent += mass;
	CounterPrint();
}



//-----------------------------------------------------------------------------
/*
N:\Windows\My Little Pony - Friendship is Magic - S07 720p\MLP-FiM S07E01 Celest
ial Advice 720p.mkv
>!File: D:\dick
!no backup stored
!orphan
cattag: 00000000000000001110111111111111
  Size: 1023.00 TB (1124800395214848 B)
        \___10c__/
>!File: 512 B
//      \___________31c_______________/
Backup: Create: 15 сент€бр€ 2017 13:27:03    Write: 15 сент€бр€ 2017 13:27:03
//      \_______________37c_________________/       \__________25c__________/
>!File: Create: 15 сент€бр€ 2017 13:27:03
                \____________29c____________/
Backup: md5 is not calculated                ecc v1: 243.00B (243 B)
>!File: 4b45788b2f5edaf89d405a0e42ac1b6b     ecc v1: 243.00B (243 B)
        \______________32c_____________/
*/
static std::string PrettySize( uint64 size )
{
	const std::string bytes = ToString( size ) + " B";
	if( size < 1024 ) return bytes;

	const int shortPartLen = 10;
	std::string str = FileSizeToString( size );
	str.resize( shortPartLen, ' ' );

	str += " (";
	str += bytes;
	str += ")";
	return str;
}
static std::string PrettyTime( const NB_DATE time )
{
	std::wstring str;
	DateToString( time, str );
	str.erase( str.find(L" г."), 3 );
	return WtoA( str );
}
static std::string PrettyData( const rsData& data )
{
	if( data.dataType == rsData::DT_NONE ) return "";

	std::string str = "ecc ";
	if( data.dataType == rsData::DT_32kbRAW_1mbECC ) str += "v0";
	if( data.dataType == rsData::DT_12kbECC_1mbDATA ) str += "v1";
	str += ": ";
	str += PrettySize( data.data.size() );

	return str;
}
static std::string PrettyMd5( const rsFile& file )
{
	const int md5Len = 37;
	std::string md5;

	if( file.md5String.empty() )
		md5 = "md5 is not calculated";
	else
		md5 = file.md5String;//+ ", ";
	md5.resize( md5Len, ' ' );

	return md5;
}
void print( const rsFilePair& pair )
{
	NOT_EMPTY_PAIR( pair );
	const rsFile& file = pair.ecFile ? *pair.ecFile : *pair.realFile;
	const rsFileComparison& cmp = pair.diff;

	{
		if( !cmp.RealPresent() ) {
			std::cout << file.name << std::endl;
			std::cout << "!orphan" << std::endl;
		} else if( !cmp.EcPresent() ) {
			std::cout << file.name << std::endl;
			std::cout << "!not stored in ecf archive" << std::endl;
		} else if( !cmp.EqualName() ) {
			std::cout << "Backup: " << pair.ecFile->name << std::endl;
			std::cout << ">!File: " << pair.realFile->name << std::endl;
		} else {
			std::cout << file.name << std::endl;
		}

		std::cout << "cattag: " << cmp << std::endl;
	}

	{
		if( cmp.BothPresent() && !cmp.EqualSize() )
		{
			std::cout << "Backup: " << PrettySize( pair.ecFile->fileSize ) << std::endl;
			std::cout << ">!File: " << PrettySize( pair.realFile->fileSize ) << std::endl;
		}
		else
		{
			std::cout << "  Size: " << PrettySize( file.fileSize ) << std::endl;
		}
	}

	{
		const int createLen = 29;
		std::string create = PrettyTime( file.createDate );
		std::string write = PrettyTime( file.writeDate );
		create.resize( createLen, ' ' );

		const bool differs = cmp.BothPresent() & (!cmp.EqualWrite() | !cmp.EqualCreate());
		std::cout << (differs ? "Backup: " : "  Date: ") <<
			"Create: " << create << "Write: " << write << std::endl;

		if( differs )
		{
			const int createLenFull = createLen + 8;
			create.clear();
			write.clear();

			if( !cmp.EqualCreate() )
				create = "Create: " + PrettyTime( pair.realFile->createDate );
			if( !cmp.EqualWrite() )
				write = "Write: " + PrettyTime( pair.realFile->writeDate );

			create.resize( createLenFull, ' ' );
			std::cout << ">!File: " << create << write << std::endl;
		}
	}

	{
		if( pair.ecFile )
		{
			std::string data = PrettyData( pair.ecFile->data );
			if( data.empty() ) data = "no recovery data";
			std::cout << "Backup: " << PrettyMd5( *pair.ecFile ) << data << std::endl;
		}
		if( pair.realFile )
		{
			if( cmp.Md5Differs() || cmp.EccDiffers() )
				std::cout << ">!File: ";
			else
				std::cout << "  File: ";
			std::cout << PrettyMd5( *pair.realFile ) << PrettyData( pair.realFile->data ) << std::endl;
		}
	}
}



//-----------------------------------------------------------------------------
void print( const rsFileCategories& stat )
{
	for( std::size_t i = 0; i < FCC_ENUM_SIZE; i++ )
	{
		const FILE_COMPARISON_CATEGORY cat = FILE_COMPARISON_CATEGORY(i);
		if( stat[cat] ) std::cout << stat.Cat2Name(cat) << " - " <<
			stat.Cat2Desc(cat) << ": " << stat[cat] << std::endl;
	}
}



//-----------------------------------------------------------------------------
static const char* FCC_ENUM_NAMES[] =
{
	#define ENTRY( name, description, filter ) #name
	FCC_ENTRIES_MACRO( COMMA )
	#undef ENTRY
};
static const char* FCC_ENUM_DESCRIPTIONS[] =
{
	#define ENTRY( name, description, filter ) description
	FCC_ENTRIES_MACRO( COMMA )
	#undef ENTRY
};
FILE_COMPARISON_CATEGORY rsFileCategories::Name2Cat( const std::string& name )
{
	std::size_t cat = 0;
	for( ; cat < FCC_ENUM_SIZE; cat++ )
	{
		if( name == FCC_ENUM_NAMES[cat] ) break;
	}
	return FILE_COMPARISON_CATEGORY(cat);
}
const char* rsFileCategories::Cat2Name( FILE_COMPARISON_CATEGORY cat )
{
	ASSERT( (cat >= 0) && (cat < FCC_ENUM_SIZE) )("invalid category");
	return FCC_ENUM_NAMES[cat];
}
const char* rsFileCategories::Cat2Desc( FILE_COMPARISON_CATEGORY cat )
{
	ASSERT( (cat >= 0) && (cat < FCC_ENUM_SIZE) )("invalid category");
	return FCC_ENUM_DESCRIPTIONS[cat];
}



//-----------------------------------------------------------------------------
/*void print( const rsFile& f ) {
	std::cout << ">" << f.name << ":" << std::endl;
	std::cout << "size: " << f.fileSize << std::endl;
	std::cout << "crea: " << f.createDate << std::endl;
	std::cout << "modi: " << f.writeDate << std::endl;
}*/



//-----------------------------------------------------------------------------
/*void print( const rsFile& realFile, const rsFile& ecFile, const rsFileComparison& cmp )
{
	if( cmp.EqualName() ) {
		std::cout << "file name: '" << ecFile.name << "'" << std::endl;
	} else {
		std::cout << "file name:   >" << realFile.name << std::endl;
		std::cout << "backup name: >" << ecFile.name << std::endl;
	}

	if( !cmp.EqualSize() && cmp.EccStored() ) {
		std::cout << "file size:   >" << realFile.fileSize << std::endl;
		std::cout << "backup size: >" << ecFile.fileSize << std::endl;
	}

	if( !cmp.EqualCreate() ) {
		std::cout << "file creation date:   >" << longdate(realFile.createDate) << std::endl;
		std::cout << "backup creation date: >" << longdate(ecFile.createDate) << std::endl;
	}

	if( !cmp.EqualWrite() ) {
		std::cout << "file modify date:   >" << longdate(realFile.writeDate) << std::endl;
		std::cout << "backup modify date: >" << longdate(ecFile.writeDate) << std::endl;
	}

	if( cmp.IsOverwriten() ) {
		std::cout << "this file appears to be overwritten since last backup" << std::endl;
	}
	if( cmp.IsMoved() ) {
		std::cout << "this file appears to be moved or renamed" << std::endl;
	}
	if( cmp.IsCopied() ) {
		std::cout << "this file appears to be copied to the new location and lost its original creation date" << std::endl;
	}

	if( !cmp.EccStored() ) {
		std::cout << "ecc test: no error correction code stored in the backup" << std::endl;
	} else if( cmp.EccEqual() ) {
		std::cout << "ecc test: file is healthy" << std::endl;
	} else if( cmp.EccDiffers() ) {
		if( cmp.IsOverwriten() )
			std::cout << "ecc test: overwritten file failed error correction code comparison, as expected" << std::endl;
		else
			std::cout << "ecc test: file failed error correction code comparison!" << std::endl;
	} else {
		std::cout << "ecc test was not yet performed" << std::endl;
	}
}*/



//-----------------------------------------------------------------------------
/*void print( const rsFilePair& pair )
{
	NOT_EMPTY_PAIR( pair );
	const rsFile& file = pair.realFile ? *pair.realFile : *pair.ecFile;
	const bool both = pair.ecFile && pair.realFile;
	const rsFileComparison cmp( pair );

	if( !cmp.RealPresent() )
		std::cout << "orphan" << std::endl;
	std::cout << file.name << std::endl;
	if( !cmp.EcPresent() )
		std::cout << "no backup stored" << std::endl;
	if( both && !cmp.EqualName() )
		std::cout << ">Backuped Name: " << pair.ecFile->name << std::endl;
	if( both )
		std::cout << "diff tag: " << cmp << std::endl;

	std::cout << "Size:            " << FileSizeToString( file.fileSize ) <<
		" (" << file.fileSize << " B)" << std::endl;
	if( both && !cmp.EqualSize() )
		std::cout << ">Backuped Size:  " << FileSizeToString( pair.ecFile->fileSize ) <<
			" (" << pair.ecFile->fileSize << " B)" << std::endl;

	std::cout << "CreatDate:       " << file.createDate << std::endl;
	if( both && !cmp.EqualCreate() )
		std::cout << ">Backuped Creat: " << pair.ecFile->createDate << std::endl;
	std::cout << "WriteDate:       " << file.writeDate << std::endl;
	if( both && !cmp.EqualWrite() )
		std::cout << ">Backuped Write: " << pair.ecFile->writeDate << std::endl;

	if( pair.realFile && !pair.realFile->md5String.empty() )
		std::cout << "md5:             " << pair.realFile->md5String << std::endl;
	if( pair.ecFile && !pair.ecFile->md5String.empty() )
		std::cout << "Bacukp md5:      " << pair.ecFile->md5String << std::endl;

	if( pair.realFile && pair.realFile->data.dataType != rsData::DT_NONE )
	{
		std::cout << "ecc:             ";
		//todo: todo
		if( pair.realFile->data.dataType == rsData::DT_32kbRAW_1mbECC ) std::cout << "v0";
		if( pair.realFile->data.dataType == rsData::DT_12kbECC_1mbDATA ) std::cout << "v1";
		std::cout << ", size: " << FileSizeToString( pair.realFile->data.data.size() ) <<
			" (" << pair.realFile->data.data.size() << " B)" << std::endl;
	}

	if( pair.ecFile && pair.ecFile->data.dataType != rsData::DT_NONE )
	{
		std::cout << "Backup ecc:      ";
		//todo: todo
		if( pair.ecFile->data.dataType == rsData::DT_32kbRAW_1mbECC ) std::cout << "v0";
		if( pair.ecFile->data.dataType == rsData::DT_12kbECC_1mbDATA ) std::cout << "v1";
		std::cout << ", size: " << FileSizeToString( pair.ecFile->data.data.size() ) <<
			" (" << pair.ecFile->data.data.size() << " B)" << std::endl;
	}
}*/
