#pragma once

#include <IntrusiveShpFreeList.hpp>
#include <IntrusiveShp.hpp>
#include <BoardRef.hpp>

namespace impl {

class ScopeParams : public IntrusiveSmart::FreeListNode, public ::ScopeParams {
};

};

typedef IntrusiveSmart::Shp<impl::ScopeParams> ScopeParamsPtr;

class ScopeParamsPool : public IntrusiveSmart::FreeListBase, public BoardRef {
public:
	ScopeParamsPool(BoardInterface *brd);

	void
	add(unsigned poolDepth);

	// if 'other' is given then make a clone
	ScopeParamsPtr
	get(ScopeParamsPtr other=ScopeParamsPtr());
	
};
