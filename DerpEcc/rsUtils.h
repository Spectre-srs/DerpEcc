#ifndef __RS_UTILS__
#define __RS_UTILS__

#include <vector>

#include "nbstd.h"



//-----------------------------------------------------------------------------
class rsIStream
{
	std::istream& mStr;
	uint64 mSize;
public:
	rsIStream( std::istream& str, uint64 size )
		: mStr( str )
		, mSize( size ) {
	}
	void read( char* data, std::size_t size ) {
		mStr.read( data, size );
	}
	uint64 size() {
		return mSize;
	}
};
class rsOStream
{
	std::ostream& mStr;
	uint64 mSize;
public:
	rsOStream( std::ostream& str )
		: mStr( str )
		, mSize( 0 ) {
	}
	void write( const char* data, std::size_t size ) {
		mStr.write( data, size );
		mSize += size;
	}
	uint64 size() {
		return mSize;
	}
};



//-----------------------------------------------------------------------------
inline uint32 u64_u32( uint64 val )
{
	ENFORCE( (val & 0xFFFFFFFF00000000) == 0 )
		("!error: uint64-32 downcast overflow: ")(val);
	return static_cast<uint32>(val & 0xFFFFFFFF);
}
// todo: some x64 define is required
#if 1
inline uint32 size_t_u32( std::size_t val )
{
	return u64_u32( val );
}
inline std::size_t u64_size_t( uint64 val )
{
	return val;
}
#else
inline uint32 size_t_u32( std::size_t val )
{
	return val;
}
inline std::size_t u64_size_t( uint64 val )
{
	return u64_u32( val );
}
#endif



//-----------------------------------------------------------------------------
/*!
 * \brief
 *      Split string into tokens. Ignores empty tokens.
 *      For example, `split("/ab/cde//fgh/", "/")` will produce 
 *      three tokens: {"ab", "cde", "fgh"}
 * \param source
 *      Source string to split
 * \param delim
 *      String containing possible delimiters.
 * \return
 *      Vector with found tokens.
 */
template<typename STRING>
inline std::vector<STRING>
split( STRING const& source, STRING const& delim )
{
	std::vector<STRING> holder;
	std::size_t num_tokens = 0;

	std::size_t pos = source.find_first_not_of(delim);
	std::size_t delim_pos = source.find_first_of(delim, pos);

	while (pos != std::string::npos) {
		holder.push_back(source.substr(pos, delim_pos - pos));
		++num_tokens;

		pos = source.find_first_not_of(delim, delim_pos);
		delim_pos = source.find_first_of(delim, pos);
	}

	return holder;
}



//-----------------------------------------------------------------------------
inline std::wstring FileNameFromPath( std::wstring path )
{
	const std::size_t slashPos = path.find_last_of( L"\\/" );
	if( slashPos != std::wstring::npos )
		path.erase( 0, slashPos + 1 );

	return path;
}



//-----------------------------------------------------------------------------
inline std::wstring FilePathFromPath( std::wstring path )
{
	const std::size_t slashPos = path.find_last_of( L"\\/" );
	if( slashPos != std::wstring::npos )
		path.erase( slashPos + 1 );
	else
		path.clear();

	return path;
}



//-----------------------------------------------------------------------------
inline bool FilePathGlobal( const std::wstring& path )
{
	return path.find( L":\\" ) != std::wstring::npos;
}



//-----------------------------------------------------------------------------
inline bool FilePathMask( const std::wstring& path )
{
	return path.find_first_of( L"?*" ) != std::wstring::npos;
}



//-----------------------------------------------------------------------------
inline std::ostream& operator << ( std::ostream& left, const std::wstring& right )
{
	std::string WtoA( const std::wstring& ); //sorry
	return left << WtoA( right );
}



#endif // __RS_UTILS__
