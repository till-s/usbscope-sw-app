#pragma once

#include <memory>
#include <mutex>
#include <stdio.h>

class FreeList {
	private:
		void        *head_;
		size_t       extr_;
		size_t       ovhd_;
		size_t       nelm_;
		size_t       totl_;
		std::mutex   mutx_;

	protected:

		size_t
		setOverhead(size_t ovhd)
		{
			size_t rv = ovhd_;
			ovhd_ = ovhd;
			return rv;
		}

	public:

		FreeList(size_t extr)
		: head_( NULL ),
		  extr_( extr ),
		  ovhd_( 0    ),
		  nelm_( 0    ),
		  totl_( 0    )
		{
		}

		void
		add(unsigned n)
		{
			while ( n > 0 ) {
				push( ::operator new( extr_ + ovhd_ ) );
				{
				std::lock_guard<std::mutex> g( mutx_ );
				totl_++;
				}
				n--;
			}
		}

		void *
		pop()
		{
			void *p;
			{
			std::lock_guard<std::mutex> g( mutx_ );
			if ( ( p = head_ ) ) {
				head_ = *reinterpret_cast<void**>( head_ );
				nelm_--;
			}
			}
			printf("pop %p\n", p);
			return p;
		}

		void
		push(void *p)
		{
			if ( p ) {
				printf("Push %p (elsz %ld)\n", p, extr_ + ovhd_);
				std::lock_guard<std::mutex> g( mutx_ );
				*reinterpret_cast<void**>( p ) = head_;
				head_ = p;
				nelm_++;
			}
		}

		~FreeList()
		{
			if ( totl_ != nelm_ ) {
				// fatal error; not all elements returned
				std::terminate();
			}
			while ( void *p = pop() ) {
				::operator delete( p );
			}
		}
	};

template <typename T>
class BufPool : public FreeList {

	class BadAlloc : public std::bad_alloc {
	public:
		size_t sizeHack_;

		BadAlloc(size_t sizeHack)
		: sizeHack_( sizeHack )
		{}
	};

	template <typename TT>
	struct Alloc {
		FreeList *freeList_;

		Alloc( FreeList *fl ) : freeList_ ( fl ) {}

		Alloc( const Alloc<T> &rhs ) { freeList_ = rhs.freeList_; }

		typedef TT value_type;

		TT *
		allocate( size_t n )
		{
			printf("alloc SZ %ld\n", sizeof(TT));
			TT *rv = static_cast<TT*>( freeList_->pop() );
			if ( ! rv ) {
				throw BadAlloc( sizeof(TT) );
			}
			return rv;
		}

		void
		deallocate(TT *p, size_t n)
		{
			printf("dealloc SZ %ld\n", sizeof(TT));
			freeList_->push( static_cast<void*>( p ) );
		}
	};

public:
	typedef std::shared_ptr<T> BufPtr;

	BufPool(size_t extra)
	: FreeList( extra )
	{
		// set the correct element size in the free list
		try {
			get();
		} catch ( BadAlloc & exc ) {
			setOverhead( exc.sizeHack_ );
			printf("caught badAlloc (overhead %ld)\n", exc.sizeHack_);
		}
	}

	template<typename... Args>
	BufPtr
	get(Args &&... args)
	{
		return std::allocate_shared< T, Alloc<T> >( Alloc<T>(this), args... );
	}
};
