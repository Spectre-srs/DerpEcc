


//----------------------------------------------------------
//O_LARGEFILE

//Define the symbol _UNICODE before you build your program.
//On the Output page of the Linker folder in the project's Property Pages dialog box, set the Entry Point symbol to wWinMainCRTStartup.
//	WriteConsoleW( GetStdHandle(STD_OUTPUT_HANDLE), input.c_str(), input.size(), NULL, NULL );

//std::memset( fec_buffer_ , 0, sizeof(fec_buffer_ ) );
//"N:\\мыльница\\Beholder п8\\_(130722-151721).avi"



//----------------------------------------------------------
//со ай нид ту врайт а класс, вич вил хев operator >> (stuff) { data.pushback( stuff ); }
//вектор.резерве делает сайз >= парам

		/*std::string tmp = fileImprint.str(); //makes a copy of the string, not optimal decision
		std::ofstream out2( (ecFileName+"2").c_str(), std::ios::binary );
		out2.write( tmp.c_str(), tmp.size() );*/



/*
#include <iostream>
#include <istream>
#include <streambuf>

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

char buffer[] = "I'm a buffer with embedded nulls\0and line\n feeds";
membuf sbuf( buffer, buffer + sizeof(buffer) );
std::istream in(&sbuf);
*/



/*
#include <streambuf>
#include <istream>

struct membuf: std::streambuf {
    membuf(char const* base, size_t size) {
        char* p(const_cast<char*>(base));
        this->setg(p, p, p + size);
    }
};
struct imemstream: virtual membuf, std::istream {
    imemstream(char const* base, size_t size)
        : membuf(base, size)
        , std::istream(static_cast<std::streambuf*>(this)) {
    }
};
*/









//-----------------------------------------------------------------------------
void transpidor( std::istream& dataIn, std::size_t dataSize, std::ostream& dataOut )
{
	#define matrixSize 1024*1024 //1mb
	#define matrixRowSize 32*32
	std::vector<char> matrixIn;
	std::vector<char> matrixOut;
	matrixIn.resize( 1024*1024 );
	matrixOut.resize( 1024*1024 );
	std::size_t size = std::min<std::size_t>( matrixSize, dataSize );

	dataIn.read( &matrixIn[0], size );

	for( int x = 0; x < matrixRowSize; x++ )
	{
		for( int y = 0; y < matrixRowSize; y++ )
		{
			matrixOut[x*matrixRowSize+y] = matrixIn[y*matrixRowSize+x];
		}
	}

	dataOut.write( &matrixOut[0], size );
}



//-----------------------------------------------------------------------------
const char* tetrapidor = "tetrapidor";
void whoy()
{
	std::ofstream fstreamOut( tetrapidor, std::ios::binary );
	ENFORCE( fstreamOut.good() )("can't create file '")(tetrapidor)("'");
	std::vector<unsigned char> matrixOut;
	matrixOut.resize( 1024*1024 );
	unsigned char bzus = 0;

	for( int x = 0; x < matrixRowSize; x++ )
	{
		for( int y = 0; y < matrixRowSize; y++ )
		{
			matrixOut[x*matrixRowSize+y] = bzus++;
		}
	}

	fstreamOut.write( (char*)&matrixOut[0], 1024*1024 );
}



//-----------------------------------------------------------------------------
const char* pizdylina = "pizdylina";
void whoy2()
{
	std::ofstream fstreamOut( pizdylina, std::ios::binary );
	ENFORCE( fstreamOut.good() )("can't create file '")(pizdylina)("'");
	std::vector<unsigned char> matrixOut;
	matrixOut.resize( 1024*1024 );
	int hyesos = 1024*1024 - 32*1024;

	for( int x = 0; x < matrixRowSize; x++ )
	{
		for( int y = 0; y < matrixRowSize; y++ )
		{
			matrixOut[x*matrixRowSize+y] = x*matrixRowSize+y > hyesos ? 0x2a : 0x20;
		}
	}

	fstreamOut.write( (char*)&matrixOut[0], 1024*1024 );
}



//-----------------------------------------------------------------------------
const char* ultrasavsur = "ultrasavsur";
void bolvan()
{
	std::ifstream fstreamIn( tetrapidor, std::ios::binary );
	ENFORCE( fstreamIn.good() )("can't open file '")(tetrapidor)("'");
	std::ofstream fstreamOut( ultrasavsur, std::ios::binary );
	ENFORCE( fstreamOut.good() )("can't create file '")(ultrasavsur)("'");

	transpidor( fstreamIn, 1024*1024, fstreamOut );
}



//-----------------------------------------------------------------------------
void pidorgey()
{
	//void bacur(); bacur(); return;
			const char* uncoded = "input.txt";
			const char* coded = "coded";
			const char* decoded = "decoded";
			std::size_t size = 0;
			{
				std::ifstream fstreamIn( uncoded, std::ios::binary );
				ENFORCE( fstreamIn.good() )("can't open file '")(uncoded)("'");
				size = measureAndRewind( fstreamIn );
				if( ynPrompt() ) {
					std::ofstream fstreamOut( coded, std::ios::binary );
					ENFORCE( fstreamOut.good() )("can't create file '")(coded)("'");
					//EncodeStream( fstreamIn, size, fstreamOut );
					EncodeEccFile( fstreamIn, size, fstreamOut );
				}
			}
			{
				std::ifstream fstreamIn( uncoded, std::ios::binary );
				ENFORCE( fstreamIn.good() )("can't open file '")(uncoded)("'");
				std::ifstream fstreamInecc( coded, std::ios::binary );
				ENFORCE( fstreamInecc.good() )("can't open file '")(coded)("'");
				std::ofstream fstreamOut( decoded, std::ios::binary );
				ENFORCE( fstreamOut.good() )("can't create file '")(decoded)("'");
				//RestoreStream( fstreamIn, size, fstreamInecc, fstreamOut );
				DecodeEccFile( fstreamInecc, measureAndRewind(fstreamInecc), fstreamOut );
			}
}
void blya()
{
			const char* coded = "coded";
				std::fstream a( coded, std::ios::binary | std::ios::in | std::ios::out );
				ENFORCE( a.good() )("can't open file '")(coded)("'");
					a.seekp( 0, std::ios::beg );
				for( int i = 0; i < 1024; i++ )
				{
					char w = 0;
					a.write( &w, 1 );
				}
}



//-----------------------------------------------------------------------------
struct targaFile
{
	short width;
	short height;
	std::ofstream file;
	bool open( const char* fileName )
	{
		file.open( fileName, std::ios::binary );
		if( file ) 	//tga header
		{
			uint32 typewriter = 0;
			file.write( (char*)&typewriter, 2 );
			typewriter = 2;
			file.write( (char*)&typewriter, 1 );	// uncompressed ARGB
			typewriter = 0;
			file.write( (char*)&typewriter, 4 );
			file.write( (char*)&typewriter, 1 );
			file.write( (char*)&typewriter, 4 );
			file.write( (char*)&width, 2 );
			file.write( (char*)&height, 2 );
			typewriter = 32;
			file.write( (char*)&typewriter, 1 );	//32 bit per pixel
			typewriter = 40;
			file.write( (char*)&typewriter, 1 );	//8 for alpha, | 32 for y axis
		}
		return !!file;
	}
	void write( const uint32* data, uint32 size = 0 )
	{
		if( !size ) size = width * height;
		for( uint32 i = 0; i < size; i++ ) {
			file.write( (char*)&data[i], 4 );
		}
	}
};
template <std::size_t rows, std::size_t columns>
void Raspidoraser( uint32 *inData, uint32 *outData )
{
	uint32 x = 0, y = 0;
	for( uint32 row = 0; row < rows; row++ )
	{
		for( uint32 col = 0; col < columns; col++ )
		{
			outData[y*columns+x] = inData[row*columns+col];
			if( ++y >= rows ) { y = 0; x++; }
		}
	}
}
void targa( const uint32* data, uint32 rows, uint32 columns, const char* name )
{
	targaFile tga;
	tga.height = rows;
	tga.width = columns;
	ENFORCE( tga.open(name) )("can't open file '")(name)("'");
	tga.write( data );
}
//1023 * 1024 = 1047552 / 992/4(28/4)
//#define arrsize( array ) (sizeof(array) / sizeof(array[0]))
void iscvetayerdun()
{
	const uint16 rows = 249;
	const uint16 columns = 4211;
	const uint16 datalen = 249;
	uint32 *lobzyr = new uint32[rows*columns + 6000];
	uint32 *savsur = new uint32[rows*columns + 6000];
	memset( lobzyr, 0, sizeof(lobzyr) );
	uint32 clr[8] = { 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffff00ff, 0xff00ffff, 0xffffff00, 0xffffffff, 0xff000000 };

	for( uint32 i = 0, e = datalen*arrsize(clr); i < e; i++ ) {
		lobzyr[i] = clr[i/datalen];
	}
	targa( lobzyr, 1024, 1024, "1.tga" );//rows, columns
	Raspidoraser<rows,columns>( lobzyr, savsur );
	for( uint32 i = 0, e = 1024; i < e; i++ ) {
		savsur[i] = 0xff555555;
	}/*
	targa( savsur, 1024, 1024, "2.tga" );*/
	Raspidoraser<columns,rows>( savsur, lobzyr );
	targa( savsur, 1024, 1024, "3.tga" );
	/*for( uint32 i = 0, e = 1024*4; i < e; i++ ) {
		lobzyr[i] = 0xffff0000;
	}
	Raspidoraser<rows,columns>( lobzyr, savsur );
	targa( savsur, rows, columns, "ecc 4kb erasure transposed.tga" );*/
}



//-----------------------------------------------------------------------------
#include <ctime>
void chislopidor()
{
	/*const int dataAmount = 1024*1024;

	for( int eccAmount = 1024*4*3*2; eccAmount < 1024*60; eccAmount++ )
	{
		if( eccAmount%1024==0 ) std::cout << "--------" << eccAmount/1024 << "/60     //" << std::time(0) << std::endl;
		for( int pwr = 4; pwr < pow(2.f,15); pwr *= 2 )
		{
			for( int dat = pwr - 2, ecc = 2; dat >= ecc; dat--, ecc++ )
			{
				if( dataAmount / dat != eccAmount / ecc ) continue;
				if( dataAmount % dat != 0 ) continue;
				if( eccAmount % ecc != 0 ) continue;
				std::cout << dat << " + " << ecc << " = " << pwr << ", eccAmount = " << eccAmount << std::endl;
			}
		}
	}*/


	const int blockSize = 1020;

	for( int chunkSize = 1024*1024; chunkSize > 1024*900; chunkSize -= 1024 )
	{
		for( int eccPart = 16; eccPart < 37; eccPart += 4 )
		{
			if( chunkSize % (blockSize-eccPart) != 0 ) continue;

			for( int rows = blockSize; rows < chunkSize - blockSize; rows++ )
			{
				if( chunkSize % rows != 0 ) continue;
				int columns = chunkSize / rows;
				if( columns < 100 ) continue;
				std::cout << rows << " * " << columns << " = " << chunkSize << " / " << blockSize-eccPart << "(" << eccPart << ")" << std::endl;
			}
		}
	}
}



//-----------------------------------------------------------------------------
			{
				rsFile f;
				f.name = L"DerpEcc_d.pdb";
				std::ifstream fstreamIn( "DerpEcc_d.pdb", std::ios::binary );
				ENFORCE( fstreamIn.good() )("can't open file '")("DerpEcc_d.pdb")("'");
				CounterSetTargetMass( measureAndRewind(fstreamIn), "  - ecc calculation" );
				CalculateEcc( f, rsData::DT_12kbECC_1mbDATA );
				std::cout << f.md5String << std::endl;
			}
			{
				std::string ryr;
				std::ifstream fstreamIn( "DerpEcc_d.pdb", std::ios::binary );
				ENFORCE( fstreamIn.good() )("can't open file '")("DerpEcc_d.pdb")("'");
				CounterSetTargetMass( measureAndRewind(fstreamIn), "  - md5 calculation" );
				//CalculateMD5( fstreamIn, measureAndRewind(fstreamIn), ryr );
				std::cout << ryr << std::endl;
			}



//======================================================
//					* KRYA *
//======================================================



//-----------------------------------------------------------------------------
class MenuManualMeta
{
	rsFile& realFile;
	rsFile& ecFile;

public:
	MenuManualMeta( rsFile& realFile, rsFile& ecFile )
		: realFile(realFile), ecFile(ecFile)
	{
		help();
		go();
	}

	void help()
	{
		sepaline();
		
		if( realFile.name != ecFile.name ) {
			std::cout << "realname - update backup name to '" << realFile.name << "'" << std::endl;
//todo			std::cout << "backname - rename file back to '" << ecFile.name << "'" << std::endl;
		}

		if( realFile.createDate != ecFile.createDate ) {
			std::cout << "realcreate - update backup creation time to '" << realFile.createDate << "'" << std::endl;
			std::cout << "backcreate - restore file creation time to '" << ecFile.createDate << "'" << std::endl;
		}

		if( realFile.writeDate != ecFile.writeDate ) {
			std::cout << "realwrite - update backup write time to '" << realFile.writeDate << "'" << std::endl;
			std::cout << "backwrite - restore file write time to '" << ecFile.writeDate << "'" << std::endl;
		}

		std::cout << "close - return to the previous menu" << std::endl;

		std::string status = "edit metadata: " + WtoA(realFile.name);
		SetTitle( status.c_str() );
	}

	void go()
	{
		std::string cmd;
		std::wstring params;
		while( true )
		{
			if( realFile.name == ecFile.name &&
				realFile.createDate == ecFile.createDate &&
				realFile.writeDate == ecFile.writeDate ) {
					break;
			}

			GetCmd( cmd, params );
			if( cmd == "close" ) break;

			if( cmd == "realname" || cmd == "rn" )
			{
				ecFile.name = realFile.name;
				ecFile.dirty = true;
			} else

			/*if( cmd == "backname" || cmd == "bn" )
			{
				realFile.name = ecFile.name;
			} else*/

			if( cmd == "realcreate" || cmd == "rc" )
			{
				ecFile.createDate = realFile.createDate;
				ecFile.dirty = true;
			} else

			if( cmd == "backcreate" || cmd == "bc" )
			{
				realFile.createDate = ecFile.createDate;
				realFile.dirty = true;
			} else

			if( cmd == "realwrite" || cmd == "rw" )
			{
				ecFile.writeDate = realFile.writeDate;
				ecFile.dirty = true;
			} else

			if( cmd == "backwrite" || cmd == "bw" )
			{
				realFile.writeDate = ecFile.writeDate;
				realFile.dirty = true;
			} else

			{
				continue;
			}

			help();
		}
	}
};
