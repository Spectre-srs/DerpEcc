#ifndef __RS_FILE_PAIR__
#define __RS_FILE_PAIR__

#include "nbstd.h"
#include "rsFile.h"
#include <bitset>



//-----------------------------------------------------------------------------
#define COMMA ,
#define SEMICOLON ;
//ENTRY( name, description, filter )
#define FCC_ENTRIES_MACRO( SEP ) \
	ENTRY( fullequal,   "healthy files with both correct md5 and ecc", \
		d.EqualMeta() && d.Md5Equal() && d.EccEqual() ) SEP \
	ENTRY( healthy,     "healthy files with correct md5 and ecc available", \
		d.EqualMeta() && d.Md5Equal() && !d.EccCalculated() && d.EccStored() ) SEP \
	ENTRY( healthymd5,  "healthy files with correct md5 and no ecc", \
		d.EqualMeta() && d.Md5Equal() && !d.EccCalculated() && !d.EccStored() ) SEP \
	ENTRY( damaged,     "files with incorrect md5", \
		d.EqualMeta() && d.Md5Differs() && !d.EccEqual() ) SEP \
	ENTRY( legacyok,    "healthy files with correct ecc, but md5 not stored", \
		d.EqualMeta() && d.Md5Calculated() && !d.Md5Stored() && d.EccCompatible() && d.EccEqual() ) SEP \
	ENTRY( legacy,      "files with ecc backup, but no md5", \
		d.EqualMeta() && !d.Md5Calculated() && !d.Md5Stored() && !d.EccCalculated() && d.EccStored() ) SEP \
	ENTRY( recalc,      "healthy files with correct md5 and different type of ecc", \
		d.Md5Equal() && d.EccCalculated() && d.EccStored() && !d.EccCompatible() ) SEP \
	ENTRY( updated,     "files with different md5 and newer write date", \
		d.IsOverwriten() && d.Md5Differs() && !d.EccEqual() ) SEP \
	ENTRY( metaderp,    "files recognized by md5 but with different meta", \
		!d.EqualMeta() && d.EqualSize() && d.Md5Equal() && !d.EccDiffers() ) SEP \
	ENTRY( ready,       "matching files awaiting md5 check", \
		d.EqualMeta() && d.Md5Stored() && !d.Md5Calculated() ) SEP \
	ENTRY( match,       "matching files with meta-only backup", \
		d.EqualMeta() && d.RealHasSize() && !d.Md5Stored() && !d.EccStored() ) SEP \
	ENTRY( match0,      "matching zero-size files", \
		d.EqualMeta() && !d.RealHasSize() && !d.Md5Stored() && !d.EccStored() ) SEP \
	ENTRY( moved,       "files recognized by meta under different names and directories", \
		d.IsMoved() /*&& !d.Md5Calculated()*/ ) SEP \
	ENTRY( copied,      "files with meta suggesting creation date being lost due to copy", \
		d.IsCopied() /*&& !d.Md5Calculated()*/ ) SEP \
	ENTRY( overwritten, "files with meta suggesting update since last backup", \
		d.IsOverwriten() && !d.Md5Calculated() ) SEP \
	ENTRY( added,       "blank files without backup", \
		!d.EcPresent() && d.RealHasSize() && !d.Md5Calculated() && !d.EccCalculated() ) SEP \
	ENTRY( added0,      "zero-size files without backup", \
		!d.EcPresent() && !d.RealHasSize() ) SEP \
	ENTRY( unstored,    "calculated files without backup", \
		!d.EcPresent() && d.Md5Calculated() && d.EccCalculated() ) SEP \
	ENTRY( unstoredmd5, "only-md5 calculated files without backup", \
		!d.EcPresent() && d.Md5Calculated() && !d.EccCalculated() ) SEP \
	ENTRY( upgrade,    "calculated files with md5-only backup", \
		d.EqualMeta() && d.Md5Equal() && d.EccCalculated() && !d.EccStored() ) SEP \
	ENTRY( orphan,      "files not found", \
		!d.RealPresent() ) SEP \
	ENTRY( derped,      "files with unaccounted diff variation", \
		true )



//-----------------------------------------------------------------------------
enum FILE_COMPARISON_CATEGORY
{
	#define ENTRY( name, description, filter ) name
	FCC_ENTRIES_MACRO( COMMA ),
	#undef ENTRY

	FCC_ENUM_SIZE
};



//-----------------------------------------------------------------------------
struct rsFileComparison
{
	rsFileComparison()
		: cmp( EMPTY ) {
	}
	rsFileComparison( const rsFile* realFile, const rsFile* ecFile ) {
		Compare( realFile, ecFile );
	}

	void Compare( const rsFile* realFile, const rsFile* ecFile )
	{
		//ASSERT( realFile || ecFile )("empty file pair detected");
		uint32 state = EMPTY;
		static_assert( sizeof(state) == sizeof(EMPTY), "enum holder type overflow" );

		if( realFile )
		{
			state |= REAL_PRESENT;
			if( realFile->fileSize > 0 ) state |= REAL_HAS_SIZE;
			if( realFile->md5String.size() ) state |= REAL_HAS_MD5;
			if( realFile->data.data.size() ) state |= REAL_HAS_ECC;
		}

		if( ecFile )
		{
			state |= EC_PRESENT;
			if( ecFile->fileSize > 0 ) state |= EC_HAS_SIZE;
			if( ecFile->md5String.size() ) state |= EC_HAS_MD5;
			if( ecFile->data.data.size() ) state |= EC_HAS_ECC;
		}

		if( realFile && ecFile )
		{
			if( realFile->name == ecFile->name ) state |= EQUAL_NAME;
			if( realFile->fileSize == ecFile->fileSize ) state |= EQUAL_SIZE;
			if( realFile->createDate == ecFile->createDate ) state |= EQUAL_CREA;
			if( realFile->writeDate == ecFile->writeDate ) state |= EQUAL_WRIT;
			else if( realFile->writeDate > ecFile->writeDate ) state |= DATE_IS_NEWER;

			if( realFile->md5String.size() && ecFile->md5String.size() )
			{
				if( realFile->md5String == ecFile->md5String ) state |= EQUAL_MD5;
			}

			if( realFile->data.data.size() && ecFile->data.data.size() )
			{
				if( realFile->data.dataType == ecFile->data.dataType ) state |= EQUAL_ECC_TYPE;
				if( realFile->data.data == ecFile->data.data ) state |= EQUAL_ECC;
			}
		}

		cmp = FILE_META_COMPARISON( state );

		const rsFileComparison& d = *this;//remove d. from filters if all good

		#define ENTRY( name, description, filter ) \
			if( filter ) { cat = name; return; }
		FCC_ENTRIES_MACRO( SEMICOLON );
		#undef ENTRY

		ASSERT( false )("no default FILE_COMPARISON_CATEGORY filter for diff: \n")(*this);
	}

	bool RealPresent()   const { return !!(cmp & REAL_PRESENT); }
	bool RealHasSize()   const { return !!(cmp & REAL_HAS_SIZE); }
	bool EcPresent()     const { return !!(cmp & EC_PRESENT); }
	bool EcHasSize()     const { return !!(cmp & EC_HAS_SIZE); }
	bool BothPresent()   const { return RealPresent() && EcPresent(); }

	bool EqualName()     const { return !!(cmp & EQUAL_NAME); }
	bool EqualSize()     const { return !!(cmp & EQUAL_SIZE); }
	bool EqualCreate()   const { return !!(cmp & EQUAL_CREA); }
	bool EqualWrite()    const { return !!(cmp & EQUAL_WRIT); }
	bool EqualMeta()     const { return EqualName() && EqualSize() && EqualCreate() && EqualWrite(); }

	bool EccCalculated() const { return !!(cmp & REAL_HAS_ECC); }
	bool EccStored()     const { return !!(cmp & EC_HAS_ECC); }
	bool EccCompatible() const { return !!(cmp & EQUAL_ECC_TYPE); }
	bool EccEqual()      const { return !!(cmp & EQUAL_ECC); }
	bool EccDiffers()    const { return EccCalculated() && EccStored() && !EccEqual(); }

	bool Md5Calculated() const { return !!(cmp & REAL_HAS_MD5); }
	bool Md5Stored()     const { return !!(cmp & EC_HAS_MD5); }
	bool Md5Equal()      const { return !!(cmp & EQUAL_MD5); }
	bool Md5Differs()    const { return Md5Calculated() && Md5Stored() && !Md5Equal(); }

	bool IsReadyForScrub() const {
		return EqualMeta() && !Md5Calculated() && Md5Stored();
	}
	bool IsBinarySame() const {
		return EqualSize() && Md5Equal() && !EccDiffers();
	}
	bool IsOverwriten() const {
		return EqualName() && EqualCreate() && (cmp & DATE_IS_NEWER);// && !EccEqual();
	}
	bool IsMoved() const {
		return EqualSize() && !EqualName() && !EccDiffers() &&
			((EqualCreate() && EqualWrite() && !Md5Differs()) || Md5Equal());
	}
	bool IsCopied() const {
		return EqualSize() && !EqualCreate() && !EccDiffers() &&
			((EqualName() && EqualWrite() && !Md5Differs()) || Md5Equal());
	}

	operator FILE_COMPARISON_CATEGORY () const {
		return cat;
	}

private:
	enum FILE_META_COMPARISON
	{
		EMPTY          = 0,

		REAL_PRESENT   = 1 << 0,
		REAL_HAS_SIZE  = 1 << 1,
		REAL_HAS_MD5   = 1 << 2,
		REAL_HAS_ECC   = 1 << 3,

		EC_PRESENT     = 1 << 4,
		EC_HAS_SIZE    = 1 << 5,
		EC_HAS_MD5     = 1 << 6,
		EC_HAS_ECC     = 1 << 7,

		EQUAL_NAME     = 1 << 8,
		EQUAL_SIZE     = 1 << 9,
		EQUAL_CREA     = 1 << 10,
		EQUAL_WRIT     = 1 << 11,
		DATE_IS_NEWER  = 1 << 12,
		EQUAL_MD5      = 1 << 13,
		EQUAL_ECC_TYPE = 1 << 14,
		EQUAL_ECC      = 1 << 15
	};

	FILE_META_COMPARISON cmp;
	FILE_COMPARISON_CATEGORY cat;

	friend //std::ostream& operator << ( std::ostream&, const rsFileComparison& );
		std::ostream& operator << ( std::ostream& left, const rsFileComparison& right ) {
		std::bitset<sizeof(right.cmp)*8> x(right.cmp);
		return left << x;
	}
};
/*#include <bitset> //todo: move to .toString() ?
inline std::ostream& operator << ( std::ostream& left, const rsFileComparison& right ) {
	std::bitset<sizeof(right.cmp)*8> x(right.cmp);
	return left << x;
}*/



//-----------------------------------------------------------------------------
struct rsFilePair
{
private:
	typedef std::vector<uniq_ptr<rsFile>>& vecref;
	vecref ecFiles;
	vecref realFiles;
public:
	rsFile* realFile;
	rsFile* ecFile;
	rsFileComparison diff;

	rsFilePair( vecref rvec, vecref ecvec )
		: realFile( nullptr )
		, ecFile( nullptr )
		, realFiles( rvec )
		, ecFiles( ecvec ) {
	}

	rsFilePair( rsFile* real, rsFile* ec, vecref rvec, vecref ecvec )
		: realFile( real )
		, ecFile( ec )
		, realFiles( rvec )
		, ecFiles( ecvec ) {
		Update();
	}

	void Update() {
		diff.Compare( realFile, ecFile );
	}

//govnokod below:
	rsFilePair& operator = ( const rsFilePair& right ) {
		memcpy( this, &right, sizeof(right) );
		return *this;
	}

	#define NOT_EMPTY_PAIR( pair ) \
		ASSERT( pair.realFile || pair.ecFile )("empty file pair detected")
};



//-----------------------------------------------------------------------------
struct rsFileCategories
{
	rsFileCategories() {
		memset( mCount, 0, sizeof(mCount) );
	}

	template<typename FilePairsList>
	void Count( const FilePairsList& list ) {
		for( std::size_t i = 0, e = list.size(); i < e; i++ )
			Add( list[i] );
	}

	std::size_t operator [] ( FILE_COMPARISON_CATEGORY cat ) const {
		ASSERT( (cat >= 0) && (cat < FCC_ENUM_SIZE) )("access malfunction");
		return mCount[cat];
	}

	static FILE_COMPARISON_CATEGORY Name2Cat( const std::string& name );
	static const char* Cat2Name( FILE_COMPARISON_CATEGORY cat );
	static const char* Cat2Desc( FILE_COMPARISON_CATEGORY cat );

private:
	std::size_t mCount[FCC_ENUM_SIZE];

	void Add( const rsFilePair& pair ) {
		const FILE_COMPARISON_CATEGORY cat = pair.diff;
		ASSERT( cat < FCC_ENUM_SIZE )("counter malfunction");
		mCount[cat]++;
	}

	void Add( const rsFilePair* pair ) {
		Add( *pair );
	}
};



//-----------------------------------------------------------------------------
/*
#undef COMMA
#undef SEMICOLON
#undef FCC_ENTRIES_MACRO
*/



#endif __RS_FILE_PAIR__
