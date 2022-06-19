#ifndef __RS_ECF_ARCHIVE__
#define __RS_ECF_ARCHIVE__

#include <vector>

#include "nbstd.h"
#include "rsFile.h"



//-----------------------------------------------------------------------------
struct rsEcfArchive
{
	std::wstring fsName;
	std::vector<uniq_ptr<rsFile>> ecFiles;
	bool dirty;

	rsEcfArchive()
		: dirty( false ) {
	}
	rsEcfArchive( const std::wstring& filePathName )
		: dirty( false )
		, fsName( filePathName ) {
	}

	bool Dirty()
	{
		if( dirty ) return true;
		for( std::size_t i = 0, e = ecFiles.size(); i < e; i++ )
		{
			if( ecFiles[i]->dirty ) return dirty = true;
		}

		return false;
	}
	void Clear()
	{
		dirty = false;
		for( std::size_t i = 0, e = ecFiles.size(); i < e; i++ )
		{
			ecFiles[i]->dirty = false;
		}
		ASSERT( !Dirty() );
	}
};



//-----------------------------------------------------------------------------
void ReadEcfArchive( rsEcfArchive& ecfArchive );
void WriteEcfArchive( rsEcfArchive& ecfArchive, const std::wstring& path );
inline void WriteEcfArchive( rsEcfArchive& ecfArchive ) {
	WriteEcfArchive( ecfArchive, ecfArchive.fsName );
}



#endif __RS_ECF_ARCHIVE__
