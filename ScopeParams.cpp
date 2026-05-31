/**LB-MIT
 *
 * MIT License
 *
 * Copyright (c) 2026 Till Straumann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **LE-MIT*/

#include <stdexcept>

#include <scopeSup.h>
#include <ScopeParams.hpp>

#include <IntrusiveShpFreeList.hpp>
#include <IntrusiveShp.hpp>
#include <BoardRef.hpp>

unsigned impl::ScopeParams::getDecimation() const {
	return acqParams.cic0Decimation * acqParams.cic1Decimation;
}

ScopeParamsPtr
impl::ScopeParams::clone() const
{
	auto pool = static_cast<ScopeParamsPool*>( list() );
	// unlike std::shared_ptr we can simply create a new intrusive SHP out
	// of thin air - because it knows how to access the control-block/refcnt
	ScopeParamsCPtr other( this );
	return pool->get( other );
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
		fprintf(stderr, "ScopeParamsPool exhausted - consider reconfiguring it\n");
		throw std::bad_alloc();
	}
	if ( other ) {
		* static_cast<::ScopeParams*>(rv.get()) =  *other;
	}
	return rv;
}
