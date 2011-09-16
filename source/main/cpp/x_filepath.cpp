#include "xbase\x_target.h"
#include "xbase\x_types.h"
#include "xbase\x_debug.h"
#include "xbase\x_string.h"
#include "xbase\x_string_std.h"

#include "xfilesystem\x_devicealias.h"
#include "xfilesystem\x_filepath.h"
#include "xfilesystem\x_dirpath.h"
#include "xfilesystem\private\x_filesystem_common.h"

//==============================================================================
namespace xcore
{
	namespace xfilesystem
	{
		static inline char			sGetSlashChar()
		{
			return xfilesystem::isPathUNIXStyle() ? '/' : '\\';
		}
		static inline const char*	sGetDeviceEnd()
		{
			return xfilesystem::isPathUNIXStyle() ? ":/" : ":\\";
		}

		//==============================================================================
		// xfilepath: Device:\\Folder\Folder\Filename.Extension
		//==============================================================================

		xfilepath::xfilepath()
			: mString(mStringBuffer, sizeof(mStringBuffer))
		{
		}
		xfilepath::xfilepath(const char* str)
			: mString(mStringBuffer, sizeof(mStringBuffer))
		{
			mString += str;
			fixSlashes();
		}
		xfilepath::xfilepath(const xfilepath& filepath)
			: mString(mStringBuffer, sizeof(mStringBuffer))
		{
			mString += filepath.mString;
		}
		xfilepath::xfilepath(const xdirpath& dir, const xfilepath& filename)
			: mString(mStringBuffer, sizeof(mStringBuffer))
		{
			*this = filename;
			makeRelative();
			mString.insert(dir.mString);
			fixSlashes();
		}
		xfilepath::~xfilepath()
		{
		}

		void			xfilepath::clear()										{ mString.clear(); }

		s32				xfilepath::getLength() const							{ return mString.getLength(); }
		s32				xfilepath::getMaxLength() const							{ return mString.getMaxLength(); }
		bool			xfilepath::isEmpty() const								{ return mString.isEmpty(); }
		bool			xfilepath::isRooted() const
		{
			s32 pos = mString.find(sGetDeviceEnd());
			return pos >= 0;
		}

		void			xfilepath::relative(xfilepath& outRelative) const
		{
			outRelative = *this;
			outRelative.makeRelative();
		}

		void			xfilepath::makeRelative()
		{
			s32 pos = mString.find(sGetDeviceEnd());
			if (pos < 0) pos = 0;
			else         pos += 2;
			mString.remove(0, pos);
		}

		void			xfilepath::onlyFilename()
		{
			s32 slashPos = mString.rfind(sGetSlashChar());
			if (slashPos >= 0)
				mString.remove(0, slashPos+1);
		}

		xfilepath		xfilepath::getFilename() const
		{
			xfilepath p(*this);
			p.onlyFilename();
			return p;
		}

		void			xfilepath::up()
		{
			s32 lastSlashPos = mString.rfind(sGetSlashChar());
			if (lastSlashPos > 0)
			{
				s32 oneToLastSlashPos = mString.rfind(sGetSlashChar(), lastSlashPos - 1);
				if (oneToLastSlashPos < 0)
					oneToLastSlashPos = 0;
				// Remove this folder
				if (oneToLastSlashPos < lastSlashPos)
					mString.remove(oneToLastSlashPos, lastSlashPos - oneToLastSlashPos);
			}
		}

		void			xfilepath::down(const char* subDir)
		{
			s32 lastSlashPos = mString.rfind(sGetSlashChar());
			if (lastSlashPos > 0)
			{
				lastSlashPos += 1;
				xccstring s(subDir);
				mString.insert(lastSlashPos, sGetSlashChar());
				mString.insert(lastSlashPos, subDir);
				fixSlashes();
			}
			else
			{
				xccstring s(subDir);
				if (s.firstChar() != '\\' && s.firstChar() != '/')
					mString.insert(sGetSlashChar());
				mString.insert(0, subDir);
				fixSlashes();
			}
		}

		void			xfilepath::getName(xcstring& outName) const
		{
			s32 pos = mString.rfind(sGetSlashChar());
			if (pos < 0) pos = 0;
			pos++;
			s32 len = mString.rfind('.');
			if (len < 0) len = mString.getLength();
			len = len - pos;
			mString.mid(pos, outName, len);
		}
		
		void			xfilepath::getExtension(xcstring& outExtension) const
		{
			s32 pos = mString.rfind(".");
			if (pos >= 0)
				outExtension = mString.c_str() + pos;
			else
				outExtension = "";
		}

		xfiledevice*	xfilepath::getSystem(xcstring& outSystemFilePath) const
		{
			return createSystemPath(mString.c_str(), outSystemFilePath);
		}

		void			xfilepath::getDirPath(xdirpath& outDirPath) const
		{
			// Remove the filename.ext part at the end
			s32 lastSlashPos = mString.rfind(sGetSlashChar());
			if (lastSlashPos>0)
			{
				mString.substring(0, outDirPath.mString, lastSlashPos+1);
			}
			else
			{
				outDirPath.clear();
			}
		}

		bool			xfilepath::getRoot(xdirpath& outRootDirPath) const
		{
			xdirpath d;
			getDirPath(d);
			return d.getRoot(outRootDirPath);
		}

		bool			xfilepath::getParent(xdirpath& outParentDirPath) const
		{
			xdirpath d;
			getDirPath(d);
			return d.getParent(outParentDirPath);
		}

		void			xfilepath::getSubDir(const char* subDir, xdirpath& outSubDirPath) const
		{
			xdirpath d;
			getDirPath(d);
			d.getSubDir(subDir, outSubDirPath);
		}

		const char*		xfilepath::c_str() const								{ return mString.c_str(); }

		void			xfilepath::setDeviceName(const char* inDeviceName)
		{
			char deviceNameBuffer[64+2];
			xcstring deviceName(deviceNameBuffer, sizeof(deviceNameBuffer));
			getDeviceName(deviceName);
			s32 len = deviceName.getLength();
			if (inDeviceName!=NULL)
				mString.replace(0, len, inDeviceName);
			else if (len > 0)
				mString.remove(0, len + 2);
			fixSlashes(); 
			mString.trimLeft(':');
			mString.trimLeft(sGetSlashChar());
		}

		void			xfilepath::getDeviceName(xcstring& outDeviceName) const
		{
			s32 pos = mString.find(sGetDeviceEnd());
			if (pos >= 0)
				mString.left(pos, outDeviceName);
			else
				outDeviceName.clear();
		}

		void			xfilepath::setDevicePart(const char* inDevicePart)
		{
			char devicePartBuffer[64+2];
			xcstring devicePart(devicePartBuffer, sizeof(devicePartBuffer));
			getDevicePart(devicePart);
			s32 len = devicePart.getLength();
			if (inDevicePart!=NULL)
				mString.replace(0, len, inDevicePart);
			else
				mString.remove(0, len);
			fixSlashes(); 
		}

		void			xfilepath::getDevicePart(xcstring& outDevicePart) const
		{
			s32 pos = mString.find(sGetDeviceEnd());
			if (pos >= 0)
				mString.left(pos + 2, outDevicePart);
			else
				outDevicePart.clear();
		}

		xfilepath&		xfilepath::operator =  ( const char* str )				{ mString.clear(); mString += str; fixSlashes(); return *this; }
		xfilepath&		xfilepath::operator =  ( const xfilepath& path )
		{
			if (this == &path)
				return *this;
			mString = path.mString;
			return *this;
		}

		xfilepath&		xfilepath::operator += ( const char* str )				{ mString += str; fixSlashes(); return *this; }
		xfilepath&		xfilepath::operator += ( const xfilepath& str )			{ mString += str.c_str(); fixSlashes(); return *this; }

		bool			xfilepath::operator == ( const xfilepath& rhs) const	{ return (rhs.getLength() == getLength()) && x_strCompare(rhs.c_str(), c_str()) == 0; }
		bool			xfilepath::operator != ( const xfilepath& rhs) const	{ return (rhs.getLength() != getLength()) || x_strCompare(rhs.c_str(), c_str()) != 0; }

		char			xfilepath::operator [] (s32 index) const				{ return mString[index]; }

		void			xfilepath::fixSlashes()									
		{ 
			// Replace incorrect slashes with the correct one
			const char slash = sGetSlashChar();
			mString.replace(slash=='\\' ? '/' : '\\', slash);
			// Remove double slashes like '\\' or '//'
			char doubleSlash[4];
			doubleSlash[0] = slash;
			doubleSlash[1] = slash;
			doubleSlash[2] = '\0';
			doubleSlash[3] = '\0';
			mString.replace(doubleSlash, &doubleSlash[1]);
			// Remove slashes at the start and end of this string
			mString.trimDelimiters(slash, slash);
		}

		//==============================================================================
		// END xfilesystem namespace
		//==============================================================================
	};

	//==============================================================================
	// END xcore namespace
	//==============================================================================
};

//==============================================================================
// END __X_FILEPATH_H__
//==============================================================================
