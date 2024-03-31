#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>
#include <stdio.h>

class FreeList {
	private:
		void                            *head_;
		size_t                           extr_;
		size_t                           nelm_;
		size_t                           totl_;
		size_t                           size_;
		std::mutex                       mutx_;
		std::condition_variable          cond_;

	public:

		FreeList(size_t extr)
		: head_( NULL ),
		  extr_( extr ),
		  nelm_( 0    ),
		  totl_( 0    ),
		  size_( 0    )
		{
		}

		virtual void
		checkSize( size_t s )
		{
			std::lock_guard<std::mutex> g( mutx_ );
			if ( size_ && (size_ != s) ) {
				throw std::bad_alloc();
			}
			size_ = s;
		}

		virtual size_t
		getExtra()
		{
			return extr_;
		}

		virtual void
		added(size_t n)
		{
			{
			std::lock_guard<std::mutex> g( mutx_ );
			totl_ += n;
			}
		}

		virtual void *
		pop(size_t sz)
		{
			void *p;
			while ( 1 ) {
				std::unique_lock g( mutx_ );
				if ( sz > size_ ) {
					throw std::bad_alloc();
				}
				if ( ( p = head_ ) ) {
					head_ = *reinterpret_cast<void**>( head_ );
					nelm_--;
					break;
				}
				cond_.wait( g );
			}
			return p;
		}

		virtual void
		push(size_t sz, void *p)
		{
			if ( p ) {
				std::lock_guard<std::mutex> g( mutx_ );
				if ( sz < size_ ) {
					throw std::bad_alloc();
				}
				*reinterpret_cast<void**>( p ) = head_;
				head_ = p;
				nelm_++;
				cond_.notify_one();
			}
		}

		virtual 
		~FreeList() noexcept(false)
		{
			if ( totl_ != nelm_ ) {
				// fatal error; not all elements returned
				throw std::runtime_error( "Cannot destroy FreeList w/o all nodes returned!" );
				std::terminate();
			}
			while ( nelm_ > 0 ) {
				::operator delete( pop( size_ ) );
			}
		}
	};

class PoolEmpty : public std::bad_alloc {
public:
	size_t elsz_;
	PoolEmpty( size_t elsz )
	: elsz_( elsz )
	{
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
	struct ListAlloc {
		FreeList *freeList_;

		ListAlloc( FreeList *fl ) : freeList_ ( fl ) {}

		ListAlloc( const ListAlloc<T> &rhs ) { freeList_ = rhs.freeList_; }

		typedef TT value_type;

		TT *
		allocate( size_t n )
		{
			TT *rv = static_cast<TT*>( freeList_->pop( sizeof(TT) ) );
			if ( ! rv ) {
				throw PoolEmpty( sizeof(TT) );
			}
			return rv;
		}

		void
		deallocate(TT *p, size_t n)
		{
			freeList_->push( sizeof(TT), static_cast<void*>( p ) );
		}
	};

	template <typename TT>
	struct NewAlloc : public ListAlloc<TT> {
		NewAlloc( FreeList *fl ) : ListAlloc<TT> ( fl ) {}

		NewAlloc( const NewAlloc<T> &rhs ) : ListAlloc<TT>( rhs ) {}

		TT *
		allocate( size_t n )
		{
			this->freeList_->checkSize( sizeof(TT) );
			TT *rv = static_cast<TT*>( ::operator new( sizeof(TT) + this->freeList_->getExtra() ) );
			if ( ! rv ) {
				throw BadAlloc( sizeof(TT) );
			}
			return rv;
		}

		// when adding new elements avoid having to use a constructor;
		// just dont' construct/destroy
		void construct(T *) {}
		void destroy(T *) {}
	};

public:

	typedef std::shared_ptr<T> BufPtr;

	BufPool(size_t extra)
	: FreeList( extra )
	{
	}

	virtual void
	add(size_t n)
	{
		while ( n > 0 ) {
			std::allocate_shared< T, NewAlloc<T> >( NewAlloc<T>(this) );
			added(1);
			n--;
		}
	}

	// get cannot take arguments since it is used by the BufPool
	// constructor
	template <class... Args>
	BufPtr
	get(Args && ... args)
	{
		return std::allocate_shared< T, ListAlloc<T> >( ListAlloc<T>(this), args...);
	}
};

template <typename T, typename PT = std::shared_ptr<T> >
struct BufNode {
	PT	next_;
};

template <typename T, typename PT = std::shared_ptr<T> >
class BufFifo {
	std::mutex                   mutx_;
	std::condition_variable      cond_;

	PT                           head_;
	PT                           tail_;

	BufFifo(const BufFifo &)    = delete;

	BufFifo &
	operator=(const BufFifo &)  = delete;

public:
	BufFifo()
	{
	}

	void pushTail(PT el)
	{
		el->next_.reset();

		{
			std::lock_guard g( mutx_ );
			if ( tail_ ) {
				tail_->next_ = el;
				tail_        = el;
			} else {
				head_        = el;
				tail_        = el;
			}
		}
		cond_.notify_one();
	}

	PT popHead()
	{
		PT el;
		while (1 ) {
			std::unique_lock g( mutx_ );
			if ( (el = head_) ) {
				if ( ! (head_ = head_->next_) ) {
					tail_.reset();
				}
				el->next_.reset();
				return el;
			}
			cond_.wait( g );
		}
		
	}
};
