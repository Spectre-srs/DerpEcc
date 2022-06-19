#ifndef h__NB_STD
#define h__NB_STD



//-----------------------------------------------------------------------------
#include "H:/Library/C++/TESTESTERON/nbstd/macros.h"
#define static_assert(e, wat) typedef char NB_MACRO_STITCH(nb_assert, __COUNTER__)[(e)?1:-1]



//-----------------------------------------------------------------------------
#include "H:/Library/C++/TESTESTERON/nbstd/asserter.h"
#undef NB_ASSERTER_BREAK
#define NB_ASSERTER_BREAK {}
#define NB_ASSERTIONS_ENABLED true
#define ASSERT( exp )\
    if( !NB_ASSERTIONS_ENABLED ) ; else struct Assert {\
		Assert( const nb::asserter::Asserter& info )\
        {\
            static bool ignoreThis = false;\
            if( !ignoreThis && !info.Handle(__FILE__, NB_MACRO_STRINGIZE(__LINE__), #exp, ignoreThis) )\
                NB_ASSERTER_BREAK;\
        }\
    } localAssertion = nb::asserter::Asserter::Make( exp )



//-----------------------------------------------------------------------------
#include "H:/Library/C++/TESTESTERON/nbstd/enforcer.h"
#define ENFORCE(exp) *nb::enforcer::MakeEnforcer< \
	nb::enforcer::DefaultPredicate, nb::enforcer::DefaultRaiser>( (exp), \
	__FILE__ "\nline " NB_MACRO_STRINGIZE(__LINE__) ": '" #exp "'" )



//-----------------------------------------------------------------------------
#include "H:/Library/C++/TESTESTERON/nbstd/nullptr.h"
using namespace nb::cpp11_feature;



//-----------------------------------------------------------------------------
#include "H:/Library/C++/TESTESTERON/nbstd/types.h"
using namespace nb::basic_types;



//-----------------------------------------------------------------------------
#include "H:/Library/C++/TESTESTERON/nbstd/uniq_ptr.h"
using namespace nb::uniq_ptr;



#endif // h__NB_STD