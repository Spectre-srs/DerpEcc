#ifndef __RS_PRINTERS__
#define __RS_PRINTERS__

#include "nbstd.h"

struct rsFilePair;
struct rsFileCategories;



//-----------------------------------------------------------------------------
void CounterSetTargetCount( std::size_t count, const char* msg = "" );
void CounterSetTargetMass( uint64 mass, const char* msg = "" );
void CounterSetTarget( std::size_t count, uint64 mass, const char* msg = "" );
void CounterAdvanceCount( std::size_t count );
void CounterAdvanceMass( uint64 mass );
void CounterAdvance( std::size_t count, uint64 mass );



//-----------------------------------------------------------------------------
std::string FileSizeToString( uint64 bytes );
void print( const rsFilePair& pair );
void print( const rsFileCategories& stat );



#endif __RS_PRINTERS__
