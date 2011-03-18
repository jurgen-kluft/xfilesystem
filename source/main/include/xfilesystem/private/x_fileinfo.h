#ifndef __X_FILESYSTEM_FILEINFO_H__
#define __X_FILESYSTEM_FILEINFO_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase\x_types.h"

//==============================================================================
// xcore namespace
//==============================================================================
namespace xcore
{
	namespace xfilesystem
	{
		class xfiledevice;
		class xfileasync;

		struct xfileinfo
		{
			u64 				m_uByteSize;

			s32 				m_nFileIndex;
			s32 				m_nLastError;

			char*				m_szFilename;
			s32					m_sFilenameMaxLen;

			xbool				m_boReading;
			xbool				m_boWriting;
			xbool				m_boWaitAsync;
			
			xfileasync*			m_pFileAsync;
			xfiledevice*		m_pFileDevice;

			// Async IO worker thread will write here:
			volatile u32 		m_nFileHandle;

			void				clear()
			{
				m_uByteSize			= 0;

				m_nFileIndex		= 0;
				m_nLastError		= 0;

				m_szFilename		= NULL;
				m_sFilenameMaxLen	= 0;

				m_boReading			= true;
				m_boWriting			= false;
				m_boWaitAsync		= false;
				m_pFileDevice		= NULL;

				m_nFileHandle		= (u32)-1;
			}
		};

	};
	//==============================================================================
	// END xcore namespace
	//==============================================================================
};

//==============================================================================
// END __X_FILESYSTEM_FILEINFO_H__
//==============================================================================
#endif