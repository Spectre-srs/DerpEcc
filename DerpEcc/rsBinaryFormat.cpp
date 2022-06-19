#include "rsEcfArchive.h"

#include <fstream>
#include <iostream> //debug

#include "rsUtils.h"
#include "rsSchifra.h"
#include "rsPrinters.h"
#include "rsPlatform.h" //filesize



//-----------------------------------------------------------------------------
enum HEADER_TAG {
	HB_ARCHIVE1 = 0xA1,

	HT_ALPHA_ARCHIVE = 0xb1F0E1A1,
	HT_MODERN_ARCHIVE = 0xb2F0E1A1
};
static_assert( sizeof(HEADER_TAG) == 4, "HEADER_TAG must be uint32" );
static const std::size_t md5StringLength = 32;



//-----------------------------------------------------------------------------
template<typename T>
void LoadString( T& str, rsIStream& from )
{
	uint32 length;
	from.read( (char*)&length, sizeof(length) );
	str.resize( length );
	from.read( (char*)&str[0], length * sizeof(str[0]) );
}



//-----------------------------------------------------------------------------
template<typename T>
void SaveString( const T& str, rsOStream& to )
{
	uint32 length = size_t_u32( str.size() );
	to.write( (char*)&length, sizeof(length) );
	to.write( (char*)&str[0], length * sizeof(str[0]) );
}



//-----------------------------------------------------------------------------
static void ReadData_v0( rsData& data, rsIStream& from )
{
	from.read( (char*)&data.dataType, sizeof(data.dataType) );
	LoadString( data.data, from );

	// todo: eradicate old format and delete this
	if( data.dataType == rsData::DT_LEGACY ) {
		data.dataType = rsData::DT_32kbRAW_1mbECC;
		std::cout << "\btu chto volchara legacy                                ";///////////////////////////////////////////
	} else if( data.dataType == 0xCDCDCDCD ) {
		std::cout << "\bULTRASAVSUR - 0xCDCDCDCD DATA TYPE FOUND               ";///////////////////////////////////////////
		data.dataType = rsData::DT_32kbRAW_1mbECC;
	}
	if( data.data.size() == 0 && data.dataType != rsData::DT_NONE ) {
		std::cout << "\bYOU FALSHIFKA                                          ";///////////////////////////////////////////
		data.dataType = rsData::DT_NONE;
	}
}



//-----------------------------------------------------------------------------
static void WriteData_v0( rsData& data, rsOStream& to )
{
	to.write( (char*)&data.dataType, sizeof(data.dataType) );
	SaveString( data.data, to );
}



//-----------------------------------------------------------------------------
static void ReadFile_v0( rsFile& file, rsIStream& from )
{
	LoadString( file.name, from );
	from.read( (char*)&file.fileSize, sizeof(file.fileSize) );
	from.read( (char*)&file.createDate, sizeof(file.createDate) );
	from.read( (char*)&file.writeDate, sizeof(file.writeDate) );
	ReadData_v0( file.data, from );
}



//-----------------------------------------------------------------------------
static void ReadFile_v1( rsFile& file, rsIStream& from )
{
	LoadString( file.name, from );
	from.read( (char*)&file.fileSize, sizeof(file.fileSize) );
	from.read( (char*)&file.createDate, sizeof(file.createDate) );
	from.read( (char*)&file.writeDate, sizeof(file.writeDate) );

	uint8 boolean;
	from.read( (char*)&boolean, sizeof(boolean) );
	if( boolean )
	{
		file.md5String.resize( md5StringLength );
		from.read( (char*)&file.md5String[0], md5StringLength );
	}

	ReadData_v0( file.data, from );
}



//-----------------------------------------------------------------------------
static void WriteFile_v1( rsFile& file, rsOStream& to )
{
	SaveString( file.name, to );
	to.write( (char*)&file.fileSize, sizeof(file.fileSize) );
	to.write( (char*)&file.createDate, sizeof(file.createDate) );
	to.write( (char*)&file.writeDate, sizeof(file.writeDate) );

	uint8 boolean = file.md5String.empty() ? 0 : 1;
	to.write( (char*)&boolean, sizeof(boolean) );
	if( boolean )
	{
		ASSERT( file.md5String.size() == md5StringLength )
			("malformed md5 string on file '")(file.name)
			("' with length of ")(file.md5String.size());
		to.write( &file.md5String[0], md5StringLength );
	}

	WriteData_v0( file.data, to );
}



//-----------------------------------------------------------------------------
static void ReadEccSection_v1( rsEcfArchive& ecfArchive, rsIStream& from, int file_v )
{
	uint32 numFiles;
	from.read( (char*)&numFiles, sizeof(numFiles) );
	ecfArchive.ecFiles.resize( numFiles );

	CounterSetTargetCount( numFiles, "  - reading ec files" ); //statusbar
	for( uint32 i = 0; i < numFiles; i++ )
	{
		ecfArchive.ecFiles[i] = new rsFile();
		switch( file_v )
		{
			case 0:
				ReadFile_v0( *ecfArchive.ecFiles[i], from );
				break;

			case 1:
				ReadFile_v1( *ecfArchive.ecFiles[i], from );
				break;

			default:
				ENFORCE( false )("reading unknown file version ")(file_v);
		}
		CounterAdvanceCount( 1 ); //statusbar
	}
}



//-----------------------------------------------------------------------------
static void WriteEccSection_v1( rsEcfArchive& ecfArchive, rsOStream& to )
{
	uint32 numFiles = size_t_u32( ecfArchive.ecFiles.size() );
	to.write( (char*)&numFiles, sizeof(numFiles) );

	CounterSetTargetCount( numFiles, "  - writing ec files" ); //statusbar
	for( uint32 i = 0; i < numFiles; i++ )
	{
		WriteFile_v1( *ecfArchive.ecFiles[i], to );
		CounterAdvanceCount( 1 ); //statusbar
	}
}



//-----------------------------------------------------------------------------
static void ReadArchiveFormat_v1( rsEcfArchive& ecfArchive, rsIStream& from )
{
	std::stringstream buffer;
	rsOStream writeWrap( buffer );

	CounterSetTargetMass( from.size(), "  - reading archive format" ); //statusbar
	DecodeEccFile( from, writeWrap );

	const std::stringstream::pos_type errorFlag = -1;
	ENFORCE( buffer.tellp() != errorFlag )
		("buffer data stream failure, out of memory?");

	rsIStream readWrap( buffer, writeWrap.size() );
	uint32 tag;
	readWrap.read( (char*)&tag, sizeof(tag) );

	switch( tag )
	{
		case HT_ALPHA_ARCHIVE:
			ReadEccSection_v1( ecfArchive, readWrap, 0 );
			break;

		case HT_MODERN_ARCHIVE:
			ReadEccSection_v1( ecfArchive, readWrap, 1 );
			break;

		default:
			ENFORCE( false )("'")(ecfArchive.fsName)("' is not an ecfArchive!");
	}
}



//-----------------------------------------------------------------------------
static void WriteArchiveFormat_v1( rsEcfArchive& ecfArchive, rsOStream& to )
{
	std::stringstream buffer;
	rsOStream writeWrap( buffer );

	uint32 tag = HT_MODERN_ARCHIVE;
	writeWrap.write( (char*)&tag, sizeof(tag) );
	WriteEccSection_v1( ecfArchive, writeWrap );

	const std::stringstream::pos_type errorFlag = -1;
	ENFORCE( buffer.tellp() != errorFlag )
		("buffer data stream failure, out of memory?");

	rsIStream readWrap( buffer, writeWrap.size() );
	CounterSetTargetMass( readWrap.size(), "  - writing archive format" ); //statusbar
	EncodeEccFile( readWrap, to );
}



//-----------------------------------------------------------------------------
void ReadEcfArchive( rsEcfArchive& ecfArchive )
{
	ASSERT( ecfArchive.ecFiles.empty() )("reading into already filled structure");
	std::ifstream ecFileStreamIn( ecfArchive.fsName.c_str(), std::ios::binary );
	ENFORCE( ecFileStreamIn.good() )("can't open file '")(ecfArchive.fsName)("'");

	const int tag = ecFileStreamIn.peek();
	ENFORCE( tag != std::ifstream::traits_type::eof() )
		("'")(ecfArchive.fsName)("' is unreadable or zero size");

	const uint64 fileSize = ReadFileData( ecfArchive.fsName )->fileSize; //iFacepalm
	rsIStream readWrap( ecFileStreamIn, fileSize );

	if( tag == HB_ARCHIVE1 )
	{
		ReadArchiveFormat_v1( ecfArchive, readWrap );
	}
	else
	{
		// { eradicate old archives and get rid of this
		std::cout << "!possibly legacy archive" << std::endl;///////////////////////////////////////////////////////////////
		ReadEccSection_v1( ecfArchive, readWrap, 0 );
		return;// }
		ENFORCE( false )("'")(ecfArchive.fsName)("' is not an ecfArchive!");
	}
}



//-----------------------------------------------------------------------------
void WriteEcfArchive( rsEcfArchive& ecfArchive, const std::wstring& path )
{
	std::ofstream ecFileStreamOut( path.c_str(), std::ios::binary );
	ENFORCE( ecFileStreamOut.good() )("can't create file '")(path)("'");

	WriteArchiveFormat_v1( ecfArchive, rsOStream(ecFileStreamOut) );
}



//-----------------------------------------------------------------------------
/* todo: delete, there should be no archives of this pre-version in my custody
	{
		std::stringstream headerStream;
		ApplyEcc( ecFileStreamIn, sizeof(tag)+sizeof(size), ecFileStreamIn, headerStream );
		headerStream.seekg( 0, std::ios::beg );
		headerStream.read( (char*)&tag, sizeof(tag) );
		headerStream.read( (char*)&size, sizeof(size) );
		ENFORCE( tag == HT_V1_H0 )("ahtung, kurwa perdole!");
	}

	ApplyEcc( ecFileStreamIn, size, ecFileStreamIn, rawStream );

	rawStream.seekg( 0, std::ios::beg );



//-----------------------------------------------------------------------------
	std::stringstream rawStream;
	ecfArchive.Save( rawStream );
	std::size_t size = measureAndRewind( rawStream );

	{
		std::stringstream headerStream;
		headerStream.write( (char*)&tag, sizeof(tag) );
		headerStream.write( (char*)&size, sizeof(size) );
		headerStream.seekg( 0, std::ios::beg );
		CreateEcc( headerStream, sizeof(tag)+sizeof(size), ecFileStreamOut, &ecFileStreamOut );
	}

	CreateEcc( rawStream, size, ecFileStreamOut, &ecFileStreamOut );
//*/
