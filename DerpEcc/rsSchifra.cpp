#include "rsSchifra.h"

#include <iostream>
#include <fstream>

#include "import/schifra_galois_field.hpp"
#include "import/schifra_sequential_root_generator_polynomial_creator.hpp"
#include "import/schifra_reed_solomon_block.hpp"
#include "import/schifra_reed_solomon_encoder.hpp"
#include "import/schifra_reed_solomon_decoder.hpp"
#include "import/md5.h"

#include "nbstd.h"
#include "rsUtils.h"
#include "rsFile.h"
#include "rsPrinters.h"

/*
 * field_descriptor    = 8;   //amount of bits to encode per data symbol
 * gen_poly_index      = 120; //no idea, isn't required to be changed
 * code_length         = 255; //always == (2^field_descriptor - 1)
 * fec_length          = 20;  //practically == (recoverable bytes * 2)
 * gen_poly_root_count = 20;  //always == fec_length
 * primitive_polynomial 06;   //always use (field_descriptor - 2)th pair
 */



//-----------------------------------------------------------------------------
/*
	inline std::string error_as_string()
	std::size_t  errors_detected;
	std::size_t errors_corrected;
	std::size_t  zero_numerators;
	bool           unrecoverable;
*/
//1023 * 1024 = 1047552 / 992/4(28/4)
//<< current_block_index_  //todo: meaningfull errors //block.error_as_string()
//סוךעמנ - 4,00 Êֱ (4 096 באיע)
//1לב - 1024,00 Êֱ (1 048 576 באיע)
//וסס - 16 Êֱ (16 384 באיע)



//-----------------------------------------------------------------------------
//todo the struct of pidor error
/*template <std::size_t code_length, std::size_t fec_length>
bool encode_block8(
	const schifra::reed_solomon::encoder<code_length,fec_length>& encoder,
	schifra::reed_solomon::block<code_length,fec_length>& block,
	const char* dataIn,
	char* eccOut )
{
	const std::size_t data_length = code_length - fec_length;

	for( uint32 i = 0; i < data_length; ++i )
	{
		block.data[i] = (dataIn[i] & 0xFF);
	}

	bool result = encoder.encode( block );

	for( uint32 i = 0; i < fec_length; ++i )
	{
		eccOut[i] = static_cast<char>(block.fec(i) & 0xFF);
	}

	return result;
}*/



//-----------------------------------------------------------------------------
template <std::size_t rows, std::size_t columns, std::size_t length>
void Raspidoraser( char (&inData)[length], char (&outData)[length] )
{
	static_assert( rows * columns == length, "rows and columns don't add up" );

	std::size_t x = 0, y = 0;
	for( std::size_t row = 0; row < rows; row++ )
	{
		for( std::size_t col = 0; col < columns; col++ )
		{
			outData[y*columns+x] = inData[row*columns+col];
			if( ++y >= rows ) { y = 0; ++x; }
		}
	}
}



//-----------------------------------------------------------------------------
static void CalculateMD5( rsIStream& dataIn, std::string& md5Out )
{
	MD5 md5_v1;
	const uint32 blocksize = 64; //MD5::blocksize
	unsigned char buffer[blocksize];
	uint64 dataSize = dataIn.size();

	while( dataSize >= blocksize )
	{
		dataSize -= blocksize;
		dataIn.read( (char*)buffer, blocksize );
		md5_v1.update( buffer, blocksize );
		CounterAdvanceMass( blocksize ); //statusbar
	}

	if( dataSize > 0 ) {
		const uint32 leftover = u64_u32( dataSize );
		dataIn.read( (char*)buffer, leftover );
		md5_v1.update( buffer, leftover );
		CounterAdvanceMass( leftover ); //statusbar
	}

	md5_v1.finalize();
	md5Out = md5_v1.hexdigest();
}



//-----------------------------------------------------------------------------
template <std::size_t field_descriptor, std::size_t fec_length>
class rsSchifraCodec
{
	static const std::size_t gen_poly_index      = 120;
public:
	static const std::size_t code_length         = (1 << field_descriptor) - 1;
	static const std::size_t data_part           = code_length - fec_length;
	static const std::size_t ecc_part            = fec_length;

	schifra::reed_solomon::block<code_length,ecc_part> mBlock;

	rsSchifraCodec()
		: mField( field_descriptor, polynomial_size(), polynomial() )
		, mEncoder( mField, generator_polynomial(mField) )
		, mDecoder( mField, gen_poly_index ) {
	}

	bool encode_block8( const char* dataIn, char* eccOut )
	{
		for( std::size_t i = 0; i < data_part; ++i )
		{
			mBlock.data[i] = (dataIn[i] & 0xFF);
		}

		bool result = mEncoder.encode( mBlock );

		for( std::size_t i = 0; i < ecc_part; ++i )
		{
			eccOut[i] = static_cast<char>(mBlock.fec(i) & 0xFF);
		}

		return result;
	}

	bool decode_block8( const char* dataIn, const char* eccIn, char* dataOut )
	{
		for( std::size_t i = 0; i < data_part; ++i )
		{
			mBlock.data[i] = (dataIn[i] & 0xFF);
		}

		for( std::size_t i = 0; i < ecc_part; ++i )
		{
			mBlock.fec(i) = (eccIn[i] & 0xFF);
		}

		bool result = mDecoder.decode( mBlock );

		for( std::size_t i = 0; i < data_part; ++i )
		{
			dataOut[i] = static_cast<char>(mBlock[i]);
		}

		return result;
	}

	bool encode_block16( const char* dataIn_, char* eccOut_ )
	{
		const uint16* dataIn = (uint16*)dataIn_;
		uint16* eccOut = (uint16*)eccOut_;

		for( std::size_t i = 0; i < data_part; ++i )
		{
			mBlock.data[i] = (dataIn[i] & 0xFFFF);
		}

		bool result = mEncoder.encode( mBlock );

		for( std::size_t i = 0; i < ecc_part; ++i )
		{
			eccOut[i] = static_cast<uint16>(mBlock.fec(i) & 0xFFFF);
		}

		return result;
	}

	bool decode_block16( const char* dataIn_, const char* eccIn_, char* dataOut_ )
	{
		const uint16* dataIn = (uint16*)dataIn_;
		const uint16* eccIn = (uint16*)eccIn_;
		uint16* dataOut = (uint16*)dataOut_;

		for( std::size_t i = 0; i < data_part; ++i )
		{
			mBlock.data[i] = (dataIn[i] & 0xFFFF);
		}

		for( std::size_t i = 0; i < ecc_part; ++i )
		{
			mBlock.fec(i) = (eccIn[i] & 0xFFFF);
		}

		bool result = mDecoder.decode( mBlock );

		for( std::size_t i = 0; i < data_part; ++i )
		{
			dataOut[i] = static_cast<uint16>(mBlock[i]);
		}

		return result;
	}

private:
	const schifra::galois::field mField;
	const schifra::reed_solomon::encoder<code_length,ecc_part> mEncoder;
	const schifra::reed_solomon::decoder<code_length,ecc_part> mDecoder;

	static_assert( field_descriptor >= 2,  "field_descriptor out of range" );
	static_assert( field_descriptor <= 16, "field_descriptor out of range" );
	static_assert( fec_length > 0,           "fec_length out of range" );
	static_assert( fec_length < code_length, "fec_length out of range" );

	static const unsigned int polynomial_size()
	{
		switch( field_descriptor )
		{
			case 2:  return schifra::galois::primitive_polynomial_size00;
			case 3:  return schifra::galois::primitive_polynomial_size01;
			case 4:  return schifra::galois::primitive_polynomial_size02;
			case 5:  return schifra::galois::primitive_polynomial_size03;
			case 6:  return schifra::galois::primitive_polynomial_size04;
			case 7:  return schifra::galois::primitive_polynomial_size05;
			case 8:  return schifra::galois::primitive_polynomial_size06;
			case 9:  return schifra::galois::primitive_polynomial_size07;
			case 10: return schifra::galois::primitive_polynomial_size08;
			case 11: return schifra::galois::primitive_polynomial_size09;
			case 12: return schifra::galois::primitive_polynomial_size10;
			case 13: return schifra::galois::primitive_polynomial_size11;
			case 14: return schifra::galois::primitive_polynomial_size12;
			case 15: return schifra::galois::primitive_polynomial_size13;
			case 16: return schifra::galois::primitive_polynomial_size14;
			default: ;
		}
		ASSERT( false )("malformed field_descriptor");
		return 0;
	}
	static const unsigned int* polynomial()
	{
		switch( field_descriptor )
		{
			case 2:  return schifra::galois::primitive_polynomial00;
			case 3:  return schifra::galois::primitive_polynomial01;
			case 4:  return schifra::galois::primitive_polynomial02;
			case 5:  return schifra::galois::primitive_polynomial03;
			case 6:  return schifra::galois::primitive_polynomial04;
			case 7:  return schifra::galois::primitive_polynomial05;
			case 8:  return schifra::galois::primitive_polynomial06;
			case 9:  return schifra::galois::primitive_polynomial07;
			case 10: return schifra::galois::primitive_polynomial08;
			case 11: return schifra::galois::primitive_polynomial09;
			case 12: return schifra::galois::primitive_polynomial10;
			case 13: return schifra::galois::primitive_polynomial11;
			case 14: return schifra::galois::primitive_polynomial12;
			case 15: return schifra::galois::primitive_polynomial13;
			case 16: return schifra::galois::primitive_polynomial14;
			default: ;
		}
		ASSERT( false )("malformed field_descriptor");
		return 0;
	}
	static const schifra::galois::field_polynomial generator_polynomial(
		const schifra::galois::field& field )
	{
		schifra::galois::field_polynomial generator( field );

		ASSERT( schifra::make_sequential_root_generator_polynomial(
			field, gen_poly_index, ecc_part, generator) )
			("failed to create sequential root generator");

		return generator;
	}
};
static rsSchifraCodec<8,6> default255by6codec; //ecc takes *0.024 of coded data



//======================================================
//					* Algorithms *
//======================================================



//-----------------------------------------------------------------------------
class rsEccAlgorithmV0
{
	static const std::size_t ecc_part_length = default255by6codec.ecc_part;
	static const std::size_t data_part_length = default255by6codec.data_part;

public:
	void encode( rsIStream& dataIn, rsOStream& eccOut, std::string& md5Out )
	{
		MD5 md5;
		uint64 dataInLength = dataIn.size();

		while( dataInLength >= data_part_length )
		{
			dataInLength -= data_part_length;
			encode_block( dataIn, data_part_length, eccOut, md5 );
			CounterAdvanceMass( data_part_length ); //statusbar
		}

		if( dataInLength > 0 ) {
			memset( &data_buffer_[dataInLength], 0, data_part_length - dataInLength );
			encode_block( dataIn, u64_size_t(dataInLength), eccOut, md5 );
			CounterAdvanceMass( dataInLength ); //statusbar
		}

		md5.finalize();
		md5Out = md5.hexdigest();
	}
	void decode( rsIStream& dataIn, rsIStream& eccIn, rsOStream& dataOut )
	{
		uint64 dataInLength = dataIn.size();

		while( dataInLength >= data_part_length )
		{
			dataInLength -= data_part_length;
			decode_block( dataIn, data_part_length, eccIn, dataOut );
			CounterAdvanceMass( data_part_length ); //statusbar
		}

		if( dataInLength > 0 )
		{
			memset( &data_buffer_[dataInLength], 0, data_part_length - dataInLength );
			decode_block( dataIn, u64_size_t(dataInLength), eccIn, dataOut );
			CounterAdvanceMass( dataInLength ); //statusbar
		}
	}

private:
	void encode_block( rsIStream& dataIn, std::size_t dataInLength, rsOStream& eccOut, MD5& md5 )
	{
		dataIn.read( data_buffer_, dataInLength );
		md5.update( data_buffer_, size_t_u32(dataInLength) );
		if( !default255by6codec.encode_block8(data_buffer_, fec_buffer_) ) {
			std::cout << "Error during encoding of block! " << /*block_.error_as_string() <<*/ std::endl;
			return;
		}
		eccOut.write( fec_buffer_, ecc_part_length );
	}
	void decode_block( rsIStream& dataIn, std::size_t dataInLength, rsIStream& eccIn, rsOStream& dataOut )
	{
		dataIn.read( data_buffer_, dataInLength );
		eccIn.read( fec_buffer_, ecc_part_length );

		if( !default255by6codec.decode_block8(data_buffer_, fec_buffer_, data_buffer_) ) {
			std::cout << "Error during decoding of block " << /*current_block_index_ <<
			" - " << block_.error_as_string() <<*/ std::endl;
			return;
		}

		dataOut.write( data_buffer_, dataInLength );
	}

	char data_buffer_[data_part_length];
	char fec_buffer_[ecc_part_length];
};
static rsEccAlgorithmV0 ecc_v0;



//-----------------------------------------------------------------------------
class rsEccAlgorithmV1
{
	static const std::size_t ecc_part_length = default255by6codec.ecc_part;
	static const std::size_t data_part_length = default255by6codec.data_part;
	static const std::size_t blocks_count = 4211;
	static const std::size_t data_per_chunk = data_part_length * blocks_count;
	static const std::size_t ecc_per_chunk = ecc_part_length * blocks_count;

public:
	void encode( rsIStream& dataIn, rsOStream& eccOut, std::string& md5Out )
	{
		MD5 md5;
		uint64 dataInLength = dataIn.size();

		while( dataInLength >= data_per_chunk )
		{
			dataInLength -= data_per_chunk;
			encode_ecc_chunk( dataIn, data_per_chunk, eccOut, md5 );
		}

		if( dataInLength > ecc_per_chunk )
		{
			memset( &chunkRaw[dataInLength], 0, data_per_chunk - dataInLength );
			encode_ecc_chunk( dataIn, u64_size_t(dataInLength), eccOut, md5 );
		}
		else if( dataInLength > 0 )
		{
			dataIn.read( chunkRaw, u64_size_t(dataInLength) );
			md5.update( chunkRaw, u64_u32(dataInLength) );
			eccOut.write( chunkRaw, u64_size_t(dataInLength) );
			CounterAdvanceMass( dataInLength ); //statusbar
		}

		md5.finalize();
		md5Out = md5.hexdigest();
	}
	void decode( rsIStream& dataIn, rsIStream& eccIn, rsOStream& dataOut )
	{
		uint64 dataInLength = dataIn.size();

		while( dataInLength >= data_per_chunk )
		{
			dataInLength -= data_per_chunk;
			decode_ecc_chunk( dataIn, eccIn, data_per_chunk, dataOut );
		}

		if( dataInLength > ecc_per_chunk )
		{
			memset( &chunkRaw[dataInLength], 0, data_per_chunk - dataInLength );
			decode_ecc_chunk( dataIn, eccIn, u64_size_t(dataInLength), dataOut );
		}
		else if( dataInLength > 0 )
		{
			eccIn.read( chunkRaw, u64_size_t(dataInLength) );
			dataOut.write( chunkRaw, u64_size_t(dataInLength) );
			CounterAdvanceMass( dataInLength ); //statusbar
		}
	}

private:
	void encode_ecc_chunk( rsIStream& dataIn, std::size_t dataInLength, rsOStream& eccOut, MD5& md5 )
	{
		dataIn.read( chunkRaw, dataInLength );
		md5.update( chunkRaw, size_t_u32(dataInLength) );
		Raspidoraser<blocks_count,data_part_length>( chunkRaw, chunkRotated );

		std::size_t processed = dataInLength % blocks_count; //statusbar
		if( processed == 0 ) processed = dataInLength / blocks_count; //statusbar
		std::size_t chunkOffset = 0, eccOffset = 0;
		while( chunkOffset < data_per_chunk )
		{
			if( !default255by6codec.encode_block8(&chunkRotated[chunkOffset], &chunkRaw[eccOffset]) )
			{
				std::cout << "encode error - " << /*block.error_as_string() <<*/ std::endl;
				return;
			}
			chunkOffset += data_part_length;
			eccOffset += ecc_part_length;
			CounterAdvanceMass( processed ); //statusbar
			processed = dataInLength / blocks_count; //statusbar
		}
		eccOut.write( chunkRaw, eccOffset );
	}
	void decode_ecc_chunk( rsIStream& dataIn, rsIStream& eccIn, std::size_t dataInLength, rsOStream& dataOut )
	{
		dataIn.read( chunkRaw, dataInLength );
		Raspidoraser<blocks_count,data_part_length>( chunkRaw, chunkRotated );

		std::size_t processed = dataInLength % blocks_count; //statusbar
		if( processed == 0 ) processed = dataInLength / blocks_count; //statusbar
		eccIn.read( chunkRaw, ecc_per_chunk );
		std::size_t chunkOffset = 0, eccOffset = 0;
		while( chunkOffset < data_per_chunk )
		{
			if( !default255by6codec.decode_block8(&chunkRotated[chunkOffset],
				&chunkRaw[eccOffset], &chunkRotated[chunkOffset]) )
			{
				std::cout << "decode error - " << /*block.error_as_string() <<*/ std::endl;
			}
			chunkOffset += data_part_length;
			eccOffset += ecc_part_length;
			CounterAdvanceMass( processed ); //statusbar
			processed = dataInLength / blocks_count; //statusbar
		}

		Raspidoraser<data_part_length,blocks_count>( chunkRotated, chunkRaw );
		dataOut.write( chunkRaw, dataInLength );
	}

	char chunkRaw[data_per_chunk];
	char chunkRotated[data_per_chunk];
};
static rsEccAlgorithmV1 ecc_v1;



//-----------------------------------------------------------------------------
class rsEccAlgorithmV1_f
{
	static const std::size_t block_length = default255by6codec.code_length;
	static const std::size_t ecc_part_length = default255by6codec.ecc_part; //unused
	static const std::size_t data_part_length = default255by6codec.data_part;
	static const std::size_t blocks_count = 4211;
	static const std::size_t data_per_chunk = data_part_length * blocks_count;
	static const std::size_t ecc_per_chunk = ecc_part_length * blocks_count; //unused
	static const std::size_t chunk_size = data_per_chunk + ecc_per_chunk;

public:
	void encode( rsIStream& dataIn, rsOStream& dataOut )
	{
		uint64 dataInLength = dataIn.size();

		while( dataInLength >= data_per_chunk )
		{
			dataInLength -= data_per_chunk;
			encode_ecc_chunk( dataIn, data_per_chunk, dataOut );
		}

		if( dataInLength > 0 )
		{
			encode_ecc_chunk( dataIn, u64_size_t(dataInLength), dataOut );
		}
	}
	void decode( rsIStream& dataIn, rsOStream& dataOut )
	{
		uint64 dataInLength = dataIn.size();

		while( dataInLength >= chunk_size )
		{
			dataInLength -= chunk_size;
			decode_ecc_chunk( dataIn, dataOut );
		}

		if( dataInLength > 0 )
		{
			ASSERT( false )( "this shouldn't happen mmat'" );
		}
	}

private:
	void encode_ecc_chunk( rsIStream& dataIn, std::size_t dataInLength, rsOStream& dataOut )
	{
		std::size_t blockOffset = 0;
		while( dataInLength >= data_part_length )
		{
			encode_ecc_block( dataIn, data_part_length, blockOffset );
			dataInLength -= data_part_length;
			blockOffset += block_length;
		}

		if( dataInLength > 0 )
		{
			memset( &chunkRaw[blockOffset+dataInLength], 0, data_part_length - dataInLength );
			encode_ecc_block( dataIn, dataInLength, blockOffset );
		}

		Raspidoraser<block_length,blocks_count>( chunkRaw, chunkRotated );
		dataOut.write( chunkRotated, chunk_size );
	}
	void encode_ecc_block( rsIStream& dataIn, std::size_t dataInLength, std::size_t blockOffset )
	{
		dataIn.read( &chunkRaw[blockOffset], dataInLength );
		if( !default255by6codec.encode_block8(&chunkRaw[blockOffset], &chunkRaw[blockOffset+data_part_length]) )
		{
			std::cout << "encode error - " << /*block.error_as_string() <<*/ std::endl;
			return;
		}
		CounterAdvanceMass( dataInLength ); //statusbar
	}

	void decode_ecc_chunk( rsIStream& dataIn, /*std::size_t dataInLength,*/ rsOStream& dataOut )
	{
		dataIn.read( chunkRotated, chunk_size );
		Raspidoraser<blocks_count,block_length>( chunkRotated, chunkRaw );

		std::size_t chunkOffset = 0, blockOffset = 0;
		while( chunkOffset < data_per_chunk )
		{
			if( !default255by6codec.decode_block8(&chunkRaw[blockOffset],
				&chunkRaw[blockOffset+data_part_length], &chunkRotated[chunkOffset]) )
			{
				std::cout << "decode error - " << /*block.error_as_string() <<*/ std::endl;
			}
			blockOffset += block_length;
			chunkOffset += data_part_length;
			CounterAdvanceMass( data_part_length ); //statusbar
		}

		dataOut.write( chunkRotated, data_per_chunk );
	}

	char chunkRaw[chunk_size];
	char chunkRotated[chunk_size];
};
static rsEccAlgorithmV1_f file_ecc_v1;



//======================================================
//					* CHEEKI BREEKI *
//======================================================



//-----------------------------------------------------------------------------
void CalculateMD5( rsFile& realFile )
{
	ENFORCE( realFile.name.size() > 0 )("can't read md5 for file with empty name");
	ENFORCE( realFile.fileSize > 0 )("input file '")(realFile.name)("' is zero length");

	std::ifstream realFileStreamIn( realFile.name.c_str(), std::ios::binary );
	ENFORCE( realFileStreamIn.good() )("can't open file '")(realFile.name)("'");
	rsIStream realFileWrap( realFileStreamIn, realFile.fileSize );

	CalculateMD5( realFileWrap, realFile.md5String );
}



//-----------------------------------------------------------------------------
void CalculateEcc( rsFile& realFile, rsData::DATA_TYPE type )
{
	ENFORCE( realFile.name.size() > 0 )("can't read ecc for file with empty name");
	ENFORCE( realFile.fileSize > 0 )("input file '")(realFile.name)("' is zero length");

	std::ifstream realFileStreamIn( realFile.name.c_str(), std::ios::binary );
	ENFORCE( realFileStreamIn.good() )("can't open file '")(realFile.name)("'");
	rsIStream realFileWrap( realFileStreamIn, realFile.fileSize );

	std::stringstream realFileEccStream;
	rsOStream realFileEccWrap( realFileEccStream );

	switch( type )
	{
		case rsData::DT_NONE:
		{
			ENFORCE( false )("input file '")(realFile.name)("' does not have ecc type set");
		}
		case rsData::DT_LEGACY: // todo: eradicate old format and delete this branch
			std::cout << "ey blet tu ahuel tam?" << std::endl;
		case rsData::DT_32kbRAW_1mbECC:
		{
			ecc_v0.encode( realFileWrap, realFileEccWrap, realFile.md5String );
			break;
		}
		case rsData::DT_12kbECC_1mbDATA:
		{
			ecc_v1.encode( realFileWrap, realFileEccWrap, realFile.md5String );
			break;
		}
		default:
		{
			ASSERT( false )("input file '")(realFile.name)("' wants unknown ecc type '")(type)("'");
		}
	}

	const std::size_t eccLength = u64_size_t(realFileEccWrap.size());
	realFile.data.Resize( eccLength );
	realFileEccStream.read( realFile.data.Get(), eccLength );
	realFile.data.dataType = type;
}



//-----------------------------------------------------------------------------
void RestoreFromEcc( rsFile& realFile, rsFile& ecFile, const std::wstring& fixedFileName )
{
	std::ifstream realFileStreamIn( realFile.name.c_str(), std::ios::binary );
	ENFORCE( realFileStreamIn.good() )("can't read file '")(realFile.name)("'");
	rsIStream realFileWrap( realFileStreamIn, realFile.fileSize );

	std::ofstream fixedFileStreamOut( fixedFileName.c_str(), std::ios::binary );
	ENFORCE( fixedFileStreamOut.good() )("can't create file '")(fixedFileName)("'");
	rsOStream fixedFileWrap( fixedFileStreamOut );

	std::stringstream ecFileEccStream;
	ecFileEccStream.write( ecFile.data.Get(), ecFile.data.Size() );
	rsIStream ecFileWrap( ecFileEccStream, ecFile.data.Size() );

	switch( ecFile.data.dataType )
	{
		case rsData::DT_NONE:
		{
			ENFORCE( false )("input file '")(ecFile.name)("' does not have ecc stored");
		}
		case rsData::DT_LEGACY: // todo: eradicate old format and delete this branch
			std::cout << "ey blet tu ahuel tam?" << std::endl;
		case rsData::DT_32kbRAW_1mbECC:
		{
			ecc_v0.decode( realFileWrap, ecFileWrap, fixedFileWrap );
			break;
		}
		case rsData::DT_12kbECC_1mbDATA:
		{
			ecc_v1.decode( realFileWrap, ecFileWrap, fixedFileWrap );
			break;
		}
		default:
		{
			ASSERT( false )("input file '")(ecFile.name)("' wants unknown ecc type '")(ecFile.data.dataType)("'");
		}
	}
}



//-----------------------------------------------------------------------------
void EncodeEccFile( rsIStream& rawIn, rsOStream& codedOut )
{
	file_ecc_v1.encode( rawIn, codedOut );
}
void DecodeEccFile( rsIStream& codedIn, rsOStream& rawOut )
{
	file_ecc_v1.decode( codedIn, rawOut );
}
