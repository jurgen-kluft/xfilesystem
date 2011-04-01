#include "xbase\x_types.h"
#include "xbase\x_allocator.h"

#include "xunittest\xunittest.h"

#include "xfilesystem\x_filesystem.h"
#include "xfilesystem\x_threading.h"

using namespace xcore;

namespace xcore
{
	class TestHeapAllocator : public x_iallocator
	{
	public:
		TestHeapAllocator(xcore::x_iallocator* allocator)
			: mAllocator(allocator)
			, mNumAllocations(0)
		{
		}

		xcore::x_iallocator*	mAllocator;
		s32						mNumAllocations;


		const char*	name() const
		{
			return "xstring unittest test heap allocator";
		}

		void*		allocate(s32 size, s32 alignment)
		{
			++mNumAllocations;
			return mAllocator->allocate(size, alignment);
		}

		void*		callocate(s32 n_elems, s32 elem_size)
		{
			++mNumAllocations;
			return mAllocator->callocate(n_elems, elem_size);
		}

		void*		reallocate(void* mem, s32 size, s32 alignment)
		{
			return mAllocator->reallocate(mem, size, alignment);
		}

		void		deallocate(void* mem)
		{
			--mNumAllocations;
			mAllocator->deallocate(mem);
		}

		void		release()
		{
		}

		u32			usable_size(void *ptr)
		{
			return 0;
		}
	};
};

class FileSystemIoThreadObject : public xfilesystem::xthreading
{
public:
	virtual void		sleep(u32 ms)
	{
	}

	virtual bool		loop() const 
	{ 
		return false; 
	}

	virtual void		wait()
	{
	}

	virtual void		signal()
	{
	}

	virtual void		lock(u32 index)
	{
	}

	virtual void		unlock(u32 index)
	{
	}

	virtual void		wait(u32 index)
	{
	}

	virtual void		signal(u32 index)
	{
	}
};

extern x_iallocator*				sSystemAllocator;
static TestHeapAllocator			sHeapAllocator(NULL);
static FileSystemIoThreadObject		sThreadObject;

UNITTEST_SUITE_BEGIN(filesystem_init)
{
	UNITTEST_FIXTURE(main)
	{
		UNITTEST_FIXTURE_SETUP() {}
		UNITTEST_FIXTURE_TEARDOWN() {}

		// main 

		UNITTEST_TEST(init)
		{
			sHeapAllocator = xcore::TestHeapAllocator(sSystemAllocator);
			xfilesystem::init(4, &sThreadObject, &sHeapAllocator);
		}
	}
}
UNITTEST_SUITE_END


UNITTEST_SUITE_BEGIN(filesystem_exit)
{
	UNITTEST_FIXTURE(main)
	{
		UNITTEST_FIXTURE_SETUP() {}
		UNITTEST_FIXTURE_TEARDOWN() {}

		// main 

		UNITTEST_TEST(exit)
		{
			xfilesystem::exit();
		}

		UNITTEST_TEST(memory_leaks)
		{
			CHECK_TRUE(sHeapAllocator.mNumAllocations == 0);
			sHeapAllocator = xcore::TestHeapAllocator(NULL);
		}

	}
}
UNITTEST_SUITE_END