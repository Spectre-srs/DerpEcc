#ifndef __RS_FILE__
#define __RS_FILE__

#include <string>



//-----------------------------------------------------------------------------
struct rsData
{
	enum DATA_TYPE {
		DT_NONE,
		DT_LEGACY = 0x00C1FA1E, //same 0th version from the 0th version of archive, obsolete
		DT_32kbRAW_1mbECC = 0xCC1EFA32, //0th version of the ecc algorithm //32FA1ECC
		DT_12kbECC_1mbDATA = 0xCC2EA11D //1th viable version of the ecc algorithm //1DA12ECC
	};
	static const DATA_TYPE DT_DEFAULT = DT_12kbECC_1mbDATA; //default type unless otherwise specified

	DATA_TYPE dataType;

	rsData()
		: dataType( DT_NONE ) {
	}

	std::size_t Size() const {
		return data.size();
	}
	void Resize( std::size_t size ) {
		data.resize( size );
	}
	char* Get() {
		return &data[0];
	}

	bool operator == ( const rsData& right ) const {
		return (dataType == right.dataType) && (data == right.data);
	}
	bool operator != ( const rsData& right ) const {
		return (dataType != right.dataType) || (data != right.data);
	}

//private:
	std::string data;
};



//-----------------------------------------------------------------------------
//todo: todo
struct NB_DATE
{
	unsigned int hi;
	unsigned int lo;
};



//-----------------------------------------------------------------------------
struct rsFile
{
	std::wstring name;
	long long int fileSize;
	NB_DATE createDate;
	NB_DATE writeDate;
	std::string md5String;
	rsData data;

	bool dirty;

	rsFile() : dirty( false ) {};
};



//-----------------------------------------------------------------------------
inline long long int longdate( const NB_DATE& d ) {
	return (long long int(d.hi) << 32) + d.lo;
}
inline bool operator > ( const NB_DATE& left, const NB_DATE& right )
{
	return longdate(left) > longdate(right);
}
inline bool operator == ( const NB_DATE& left, const NB_DATE& right )
{
	return longdate(left) == longdate(right);
}
inline bool operator != ( const NB_DATE& left, const NB_DATE& right )
{
	return longdate(left) != longdate(right);
}
inline std::ostream& operator << ( std::ostream& left, const NB_DATE& right )
{
	return left << longdate( right );
}



#endif __RS_FILE__
