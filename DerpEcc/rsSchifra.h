#ifndef __RS_SCHIFRA__
#define __RS_SCHIFRA__

#include "rsFile.h"
#include "rsUtils.h"



//-----------------------------------------------------------------------------
void CalculateMD5( rsFile& realFile );
void CalculateEcc( rsFile& realFile, rsData::DATA_TYPE type );
void RestoreFromEcc( rsFile& realFile, rsFile& ecFile, const std::wstring& fixedFileName );



//----------------------------------------------------------------------------- v1
void EncodeEccFile( rsIStream& rawIn, rsOStream& codedOut );
void DecodeEccFile( rsIStream& codedIn, rsOStream& rawOut );



#endif __RS_SCHIFRA__
