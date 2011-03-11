#ifndef __X_FILESYSTEM_WII_H__
#define __X_FILESYSTEM_WII_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase\x_types.h"
#include "xfilesystem\private\x_filedevice.h"

//==============================================================================
// xcore namespace
//==============================================================================
namespace xcore
{
	namespace xfilesystem
	{
		extern xfiledevice*	x_CreateFileDeviceWII(EDeviceType type);
		extern void			x_DestroyFileDeviceWII(xfiledevice*);
	};

	//==============================================================================
	// END xcore namespace
	//==============================================================================
};

//==============================================================================
// END __X_FILESYSTEM_WII_H__
//==============================================================================
#endif