#include "xbase\x_target.h"
#include "xbase\x_types.h"
#include "xbase\x_allocator.h"

#include "xfilesystem\x_filesystem.h"
#include "xunittest\xunittest.h"

using namespace xcore;

UNITTEST_SUITE_LIST(xFileUnitTest);
UNITTEST_SUITE_DECLARE(xFileUnitTest, filesystem_init);
UNITTEST_SUITE_DECLARE(xFileUnitTest, xfiledevice_register);
UNITTEST_SUITE_DECLARE(xFileUnitTest, dirpath);
UNITTEST_SUITE_DECLARE(xFileUnitTest, filepath);
UNITTEST_SUITE_DECLARE(xFileUnitTest, dirinfo);
UNITTEST_SUITE_DECLARE(xFileUnitTest, fileinfo);
UNITTEST_SUITE_DECLARE(xFileUnitTest, filestream);
UNITTEST_SUITE_DECLARE(xFileUnitTest, filesystem_common);
UNITTEST_SUITE_DECLARE(xFileUnitTest, filesystem_exit);

class UnitTestAllocator : public UnitTest::Allocator
{
public:
	xcore::x_iallocator*	mAllocator;
	int						mNumAllocations;

	UnitTestAllocator(xcore::x_iallocator* allocator)
		: mNumAllocations(0)
	{
		mAllocator = allocator;
	}

	void*	Allocate(int size)
	{
		++mNumAllocations;
		return mAllocator->allocate(size, 4);
	}
	void	Deallocate(void* ptr)
	{
		--mNumAllocations;
		mAllocator->deallocate(ptr);
	}
};


xcore::x_iallocator* sSystemAllocator = NULL;

bool gRunUnitTest(UnitTest::TestReporter& reporter)
{
	sSystemAllocator = xcore::gCreateSystemAllocator();

	UnitTestAllocator unittestAllocator( sSystemAllocator );
	UnitTest::SetAllocator(&unittestAllocator);

	int r = UNITTEST_SUITE_RUN(reporter, xFileUnitTest);
	if (unittestAllocator.mNumAllocations!=0)
	{
		reporter.reportFailure(__FILE__, __LINE__, "xunittest", "memory leaks detected!");
		r = -1;
	}

	sSystemAllocator->release();
	return r==0;
}