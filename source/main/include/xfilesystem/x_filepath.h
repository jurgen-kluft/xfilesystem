#ifndef __X_FILESYSTEM_FILEPATH_H__
#define __X_FILESYSTEM_FILEPATH_H__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase\x_types.h"
#include "xbase\x_debug.h"
#include "xbase\x_string.h"

//==============================================================================
namespace xcore
{
	namespace xfilesystem
	{
		class xpath;
		class xdirpath;

		//==============================================================================
		// xfilepath: 
		//		- Relative:		Folder\Filename.Extension
		//		- Absolute:		Device:\Folder\Folder\Filename.Extension
		//==============================================================================
		class xfilepath
		{
		private:
			xstring			mBuffer;

		public:
							xfilepath();
							xfilepath(const char* str);
			explicit		xfilepath(const xstring& str);
							xfilepath(const xfilepath& filepath);
			explicit		xfilepath(const xdirpath& dir, const xfilepath& filename);
							~xfilepath();

			void			clear();

			s32				length() const;
			s32				maxLength() const;
			xbool			empty() const;
			xbool			isAbsolute() const;
			const char*		extension() const;
			const char*		relative() const;													///< Points just after device part

			void			setDevicePart(const char* deviceName);
			void			getDevicePart(char* deviceName, s32 deviceNameMaxLength) const;
			
			xfilepath&		operator =  ( const xfilepath& );

			xfilepath&		operator =  ( const char* );
			xfilepath&		operator += ( const char* );

			bool			operator == ( const xfilepath& rhs) const;
			bool			operator != ( const xfilepath& rhs) const;

			char			operator [] (s32 index) const;

			const char*		c_str() const;

		private:
			void			fixSlashes();														///< Fix slashes, replace '/' with '\'. For Unix, replace '\' with '/'.
		};

		extern xfilepath	operator + (xdirpath& dir, xfilepath& filename);

		//==============================================================================
		// END xfilesystem namespace
		//==============================================================================
	};

	//==============================================================================
	// END xcore namespace
	//==============================================================================
};

//==============================================================================
// END __X_FILESYSTEM_FILEPATH_H__
//==============================================================================
#endif