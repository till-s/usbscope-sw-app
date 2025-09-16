#include <stdexcept>

#include <scopeSup.h>
#include <ScopeParams.hpp>

#include <IntrusiveShpFreeList.hpp>
#include <IntrusiveShp.hpp>
#include <BoardRef.hpp>

unsigned impl::ScopeParams::getDecimation() const {
	return acqParams.cic0Decimation * acqParams.cic1Decimation;
}


ScopeParamsPool::ScopeParamsPool(BoardInterface *brd)
: BoardRef ( brd      )
{
}

void
ScopeParamsPool::add(unsigned poolDepth)
{
	// size of naked object
	size_t sz = sizeof(ScopeParamsPtr::element_type);
	size_t nch = scope_get_num_channels( (*this)->scope() );
	// add nch * AFEParams
	sz += nch * sizeof(static_cast<ScopeParams*>(nullptr)->afeParams[0]);
	while ( poolDepth-- ) {
		// obtain raw memory
		void *mem = operator new( sz, std::align_val_t( alignof( ScopeParamsPtr::element_type ) ) );
		if ( ! mem ) {
			throw std::bad_alloc();
		}
		// construct FreeListNode with placement new
		auto p = static_cast<ScopeParamsPtr::element_type *>(new(mem) IntrusiveSmart::FreeListNode);
		// initialize ScopeParams
		scope_init_params( (*this)->scope(), p );
		// release to free list
		put( p );
	}
}

ScopeParamsPtr
ScopeParamsPool::get(ScopeParamsCPtr other)
{
	auto rv = FreeListBase::get<typename ScopeParamsPtr::element_type>();
	if ( ! rv ) {
		// out of buffers
		throw std::bad_alloc();
	}
	if ( other ) {
		* static_cast<::ScopeParams*>(rv.get()) =  *other;
	}
	return rv;
}
