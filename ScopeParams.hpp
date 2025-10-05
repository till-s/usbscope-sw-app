#pragma once

#include <IntrusiveShpFreeList.hpp>
#include <IntrusiveShp.hpp>
#include <BoardRef.hpp>

// fwd declaration
namespace impl {
	class ScopeParams;
};

typedef IntrusiveSmart::Shp<impl::ScopeParams>       ScopeParamsPtr;
typedef IntrusiveSmart::Shp<const impl::ScopeParams> ScopeParamsCPtr;

namespace impl {


class ScopeParams : public IntrusiveSmart::FreeListNode, public ::ScopeParams {
public:
	unsigned
	getDecimation() const;

	ScopeParamsPtr
	clone() const;
};

};

class ScopeParamsPool : public IntrusiveSmart::FreeListBase, public BoardRef {
public:
	ScopeParamsPool(BoardInterface *brd);

	void
	add(unsigned poolDepth);

	// if 'other' is given then make a clone
	ScopeParamsPtr
	get(ScopeParamsCPtr other=ScopeParamsCPtr());

};
