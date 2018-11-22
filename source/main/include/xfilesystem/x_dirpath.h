#ifndef __X_FILESYSTEM_DIRPATH_H__
#define __X_FILESYSTEM_DIRPATH_H__
#include "xbase/x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase/x_debug.h"
#include "xbase/x_chars.h"

#include "xfilesystem/x_enumerator.h"

//==============================================================================
namespace xcore
{
	namespace xfilesystem
	{
		class xpath;
		class xfilepath;
		class xfilesystem;
		class xfiledevice;
		class xdevicealias;

		//==============================================================================
		// xdirpath: 
		//		- Relative:		FolderA\FolderB\
		//		- Absolute:		Device:\FolderA\FolderB\
		//==============================================================================
		class xdirpath
		{
		protected:
			friend class xfilepath;
			xfilesystem*			mParent;
			xstring					mString;

		public:
									xdirpath(xfilesystem* fs);
									xdirpath(xfilesystem* fs, xstring const& str);
									xdirpath(const xdirpath& dir);
									~xdirpath();

			void					clear();

			s32						getLength() const;
			s32						getMaxLength() const;
			bool					isEmpty() const;

			void					enumLevels(enumerate_delegate<xstring>& folder_enumerator, bool right_to_left = false) const;
			s32						getLevels() const;
			s32						getLevelOf(const xstring& folderName) const;
			s32						getLevelOf(const xdirpath& parent) const;

			bool					isRoot() const;
			bool					isRooted() const;
			bool					isSubDirOf(const xdirpath&) const;
			
			void					relative(xdirpath& outRelative) const;
			void					makeRelative();
			void					makeRelativeTo(const xdirpath& parent);

			void					up();
			void					down(const xstring& subDir);
			void					split(s32 cnt, xdirpath& parent, xdirpath& subDir) const;	///< e.g. xdirpath d("K:\\parent\\folder\\sub\\folder\\"); d.split(2, parent, sub); parent=="K:\\parent\\folder\\; sub=="sub\\folder\\";

			bool					getName(xstring& outName) const;
			bool					hasName(const xstring& inName) const;
			bool					getRoot(xdirpath& outRootDirPath) const;
			bool					getParent(xdirpath& outParentDirPath) const;
			bool					getSubDir(const xstring& subDir, xdirpath& outSubDirPath) const;

			void					setDeviceName(const xstring& deviceName);
			bool					getDeviceName(xuchars& outDeviceName) const;
			void					setDevicePart(const xstring& devicePart);
			bool					getDevicePart(xuchars& outDevicePart) const;

			const xdevicealias*		getAlias() const;
			xfiledevice*			getDevice() const;
			xfiledevice*			getSystem(xuchars& outSystemDirPath) const;

			xdirpath&				operator =  (const xdirpath&);

			xdirpath&				operator =  (const xstring&);
			xdirpath&				operator += (const xstring&);

			bool					operator == (const xstring&) const;
			bool					operator != (const xstring&) const;

			bool					operator == (const xdirpath& rhs) const;
			bool					operator != (const xdirpath& rhs) const;

			const xstring&			str() const;

		private:
			void					setOrReplaceDeviceName(xstring& ioStr, xstring const& inDeviceName) const;
			void					setOrReplaceDevicePart(xstring& ioStr, xstring const& inDeviceName) const;

			void					fixSlashes();		///< Fix slashes, replace '/' with '\'. For Unix, replace '\' with '/' (depends on xfilesystem configuration)
		};

	};

};


#endif	// __X_FILESYSTEM_DIRPATH_H__