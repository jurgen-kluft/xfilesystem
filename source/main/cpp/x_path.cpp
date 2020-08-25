#include "xbase/x_target.h"
#include "xbase/x_debug.h"
#include "xbase/x_runes.h"

#include "xfilesystem/private/x_path.h"
#include "xfilesystem/x_filepath.h"
#include "xfilesystem/x_dirpath.h"
#include "xfilesystem/x_enumerator.h"
#include "xfilesystem/private/x_devicemanager.h"

namespace xcore
{
    using namespace utf32;

    static rune   sSlash                 = '\\';
    static rune   sSemiColumnSlashStr[3] = {':', '\\', 0};
    static crunes sSemiColumnSlash(sSemiColumnSlashStr, sSemiColumnSlashStr + 2);

    static void fix_slashes(runes& str)
    {
        // Replace incorrect slashes with the correct one
        findReplace(str, (uchar32)'/', sSlash);

        // Remove double slashes like '\\' or '//'

        // Remove slashes at the start and end of this string
        trimDelimiters(str, sSlash, sSlash);
    }

    xpath::xpath()
        : m_alloc(nullptr)
        , m_path()
    {
    }

    xpath::xpath(utf32::alloc* allocator)
        : m_alloc(allocator)
        , m_path()
    {
        fix_slashes(m_path);
    }

	xpath::xpath(utf32::alloc* allocator, const utf32::crunes& path)
		: m_alloc(allocator)
	{
		utf32::copy(path, m_path, m_alloc, 16);
		fix_slashes(m_path);
	}

	xpath::xpath(utf32::alloc* allocator, const ascii::crunes& path)
		: m_alloc(allocator)
	{
		utf32::copy(path, m_path, m_alloc, 16);
		fix_slashes(m_path);
	}

    xpath::xpath(const xpath& path)
        : m_alloc(path.m_alloc)
        , m_path()
    {
        utf32::copy(path.m_path, m_path, m_alloc, 16);
		fix_slashes(m_path);
    }

    xpath::xpath(const xpath& lhspath, const xpath& rhspath) 
		: m_alloc(lhspath.m_alloc)
		, m_path()
	{
		// Combine both paths into a new path
        utf32::concatenate(m_path, lhspath.m_path, rhspath.m_path, m_alloc, 16);
	}

	xpath::~xpath()
	{
		if (m_alloc != nullptr)
		{
			m_alloc->deallocate(m_path);
		}
	}

    xpath xpath::resolve(xfilesys* fs, xfiledevice*& outdevice) const
    {
        runes rootpart = find(m_path, sSemiColumnSlash);
        if (rootpart.is_empty())
        {
            outdevice = nullptr;
        }
        else
        {
            xpath path;
            outdevice = fs->m_devman->find_device(*this, path);
            if (outdevice != nullptr)
            {
                return path;
            }
        }
        return xpath();
    }

    void xpath::clear()
	{
		m_path.clear(); 
	}

    void xpath::erase()
    {
		clear();
        if (m_alloc != nullptr)
            m_alloc->deallocate(m_path);
    }

    bool xpath::isEmpty() const { return m_path.is_empty(); }
    bool xpath::isRoot() const
    {
        runes rootpart = find(m_path, sSemiColumnSlash);
        if (rootpart.is_empty())
            return false;
        // Now determine if we are an actual root, which means that we
        // do not have any folders after our device statement.
        runes pathpart = selectUntilEndExcludeSelection(m_path, rootpart);
        return pathpart.is_empty();
    }

    bool xpath::isRooted() const
    {
        runes pos = find(m_path, sSemiColumnSlash);
        return !pos.is_empty();
    }

    bool xpath::isSubDirOf(const xpath& parent) const
    {
        // example:
        // parent = E:\data\files\music\ 
		// this = files\music\rnb\ 
		// overlap = files\music\ 
		// remainder = rnb\ 
		if (isRooted())
        {
            return false;
        }

        if (parent.isRooted())
        {
            runes overlap = selectOverlap(parent.m_path, m_path);
            return !overlap.is_empty();
        }
        return false;
    }

    void xpath::set_filepath(utf32::runes& runes, utf32::alloc* allocator)
    {
        erase();
        m_alloc = allocator;
        m_path  = runes;

        // Replace incorrect slashes with the correct one
        findReplace(m_path, (uchar32)'/', sSlash);

        // Remove double slashes like '\\' or '//'

        // Remove slash at the beginning and end
        trim(m_path, sSlash);
    }

    void xpath::set_dirpath(utf32::runes& runes, utf32::alloc* allocator)
    {
        erase();
        m_alloc = allocator;
        m_path  = runes;

        // Replace incorrect slashes with the correct one
        findReplace(m_path, (uchar32)'/', sSlash);

        // Remove double slashes like '\\' or '//'

        // Remove slash at the beginning
        trimLeft(m_path, sSlash);

        // Ensure slash at the end
        if (!ends_with(m_path, sSlash))
        {
            crunes slashstr(sSemiColumnSlashStr + 1, sSemiColumnSlashStr + 2);
            concatenate(m_path, slashstr, m_alloc, 16);
        }
    }

    void xpath::combine(const xpath& dirpath, const xpath& otherpath)
    {
        erase();
		m_alloc = dirpath.m_alloc;
        if (otherpath.isRooted())
        {
            runes relativefilepath = findSelectAfter(otherpath.m_path, sSemiColumnSlash);
            concatenate(m_path, dirpath.m_path, relativefilepath, m_alloc, 16);
        }
        else
        {
            concatenate(m_path, dirpath.m_path, otherpath.m_path, m_alloc, 16);
        }
    }

	void xpath::copy_dirpath(utf32::runes& runes)
	{
		copy(runes, m_path, m_alloc, 16);
	}

    class enumerate_runes
    {
    public:
        virtual bool operator()(s32 level, const runes& folder) = 0;
    };

    static void enumerate_fn(const runes& dirpath, enumerate_runes& enumerator, bool right_to_left)
    {
        s32   level = 0;
        runes path  = findSelectAfter(dirpath, sSemiColumnSlash);
        if (right_to_left == false)
        {
            runes dir = selectBetween(path, sSlash, sSlash);
            while (dir.is_empty() == false)
            {
                if (enumerator(level, dir) == false)
                {
                    break;
                }
                level++;
                dir = selectNextBetween(path, dir, sSlash, sSlash);
            }
        }
        else
        {
            runes dir = selectBetweenLast(path, sSlash, sSlash);
            while (dir.is_empty() == false)
            {
                if (enumerator(level, dir) == false)
                {
                    break;
                }
                level++;
                dir = selectPreviousBetween(path, dir, sSlash, sSlash);
            }
        }
    }

    class folder_counting_enumerator : public enumerate_runes
    {
    public:
        s32 mLevels;
        folder_counting_enumerator()
            : mLevels(0)
        {
        }
        virtual bool operator()(s32 level, const runes& folder)
        {
            mLevels++;
            return true;
        }
    };

    s32 xpath::getLevels() const
    {
        folder_counting_enumerator e;
        enumerate_fn(m_path, e, false);
        return e.mLevels;
    }

    bool xpath::getLevel(s32 level, xpath& outpath) const
    {
        // Return the path at level @level
        runez<4> device(":\\");

        runes path  = findSelectAfter(m_path, device);
        if (path.is_empty())
            return false;
        runes dir = selectBetween(m_path, sSlash, sSlash);
        while (level > 0 && !dir.is_empty())
        {
            dir = selectNextBetween(m_path, dir, sSlash, sSlash);
            level--;
        }
        if (!dir.is_empty())
        {
            runes path = selectUntilEndIncludeSelection(m_path, dir);
            copy(path, outpath.m_path, outpath.m_alloc, 16);
            return true;
        }
        return false;
    }

    class folder_search_enumerator : public enumerate_delegate
    {
    public:
        runes mFolderName;
        bool  mFound;
        s32   mLevel;

        folder_search_enumerator(const runes& folder)
            : mFolderName(folder)
            , mFound(false)
            , mLevel(-1)
        {
        }

        virtual bool operator()(s32 level, const runes& folder)
        {
            mFound = utf32::compare(mFolderName, folder) == 0;
            mLevel = level;
            return mFound;
        }
    };

    s32 xpath::getLevelOf(const xpath& parent) const
    {
        // PARENT:   c:\disk
        // THIS:     c:\disk\child
        // RETURN 0
        if (starts_with(m_path, parent.m_path)) {}

		//@TODO: Implement this!

		return 0;
    }

    bool xpath::split(s32 level, xpath& parent_dirpath, xpath& relative_filepath) const
    {
        // Split the path at level @level
        runes path = findSelectAfter(m_path, sSemiColumnSlash);
        if (path.is_empty())
            return false;

        runes dir = selectBetween(m_path, sSlash, sSlash);
        while (level > 0 && !dir.is_empty())
        {
            dir = selectNextBetween(m_path, dir, sSlash, sSlash);
            level--;
        }

        if (!dir.is_empty())
        {
            runes parent_path(m_path.m_str, dir.m_str, m_path.m_end);
            copy(parent_path, parent_dirpath.m_path, parent_dirpath.m_alloc, 16);
            runes relative_path = selectUntilEndIncludeSelection(m_path, dir);
            copy(relative_path, relative_filepath.m_path, relative_filepath.m_alloc, 16);
            return true;
        }

        return false;
    }

    void xpath::makeRelative()
    {
        if (isRooted())
        {
            runes pos = findSelectUntilIncluded(m_path, sSemiColumnSlash);
            removeSelection(m_path, pos);
        }
    }

    void xpath::makeRelativeTo(const xpath& parent)
    {
        makeRelative();

        // PARENT:   a\b\c\d\ 
        // THIS:     c\d\e\f\ 
		// RESULT:   e\f\ 
        runes parentpath = parent.m_path;
        if (parent.isRooted())
        {
            parentpath = findSelectAfter(parentpath, sSemiColumnSlash);
        }

        runes overlap = selectOverlap(parentpath, m_path);
        if (overlap.is_empty() == false)
        {
            runes remainder = selectUntilEndExcludeSelection(parentpath, overlap);
            keepOnlySelection(m_path, remainder);
        }
    }

    void xpath::setRootDir(const xpath& in_root_dirpath)
    {
		if (!isRooted())
		{
			runes rootpart = findSelectUntilIncluded(m_path, sSemiColumnSlash);
			if (rootpart.is_empty() == false)
			{
				insert(m_path, in_root_dirpath.m_path, m_alloc, 16);
			}
			else
			{
				replaceSelection(m_path, rootpart, in_root_dirpath.m_path, m_alloc, 16);
			}
		}
    }

    bool xpath::getRootDir(xpath& root) const
    {
		if (isRooted())
		{
			runes rootpart = findSelectUntilIncluded(m_path, sSemiColumnSlash);
			if (rootpart.is_empty() == false)
			{
				root.erase();
				copy(root.m_path, rootpart, root.m_alloc, 16);
				return true;
			}
		}
        return false;
    }

    bool xpath::getDirname(xpath& outDirPath) const
    {
        if (isEmpty())
            return false;

        outDirPath.clear();
        outDirPath.m_alloc = m_alloc;

        // Select a string until and included the last '\'
        runes dirpart = findLastSelectUntilIncluded(m_path, sSlash);
        if (dirpart.is_empty() == false)
        {
            copy(dirpart, outDirPath.m_path, outDirPath.m_alloc, 16);
            return true;
        }
        return false;
    }

    bool xpath::up()
	{
		return false;
	}
    
	bool xpath::down(xpath const& runes)
	{
		return false;
	}

    void xpath::getFilename(xpath& filename) const
    {
        runes filenamepart = findLastSelectAfter(m_path, sSlash);
        if (!filenamepart.is_empty())
        {
            filename.clear();
            concatenate(filename.m_path, filenamepart, filename.m_alloc, 16);
        }
    }

    void xpath::getFilenameWithoutExtension(xpath& filename) const
    {
        filename.clear();
        runes filenamepart = findLastSelectAfter(m_path, sSlash);
        if (!filenamepart.is_empty())
        {
            filenamepart = findLastSelectUntil(filenamepart, '.');
            concatenate(filename.m_path, filenamepart, filename.m_alloc, 16);
        }
    }

    void xpath::getExtension(xpath& filename) const
    {
        filename.clear();
        runes filenamepart = findLastSelectAfter(m_path, sSlash);
        runes filenameonly = findLastSelectUntil(filenamepart, '.');
        runes fileext      = selectUntilEndExcludeSelection(filenamepart, filenameonly);
        concatenate(filename.m_path, fileext, filename.m_alloc, 16);
    }

	xpath& xpath::operator=(const utf32::runes& path)
	{
        if (m_alloc != nullptr)
        {
			erase();
            copy(path, m_path, m_alloc, 16);
        }
        return *this;
	}
    
	xpath& xpath::operator=(const xpath& path)
    {
        if (this == &path)
            return *this;

        if (m_alloc != nullptr)
        {
            if (path.m_alloc == m_alloc)
            {
                copy(path.m_path, m_path, m_alloc, 16);
            }
            else
            { // Allocator changed
                m_alloc->deallocate(m_path);
                m_alloc = path.m_alloc;
                copy(path.m_path, m_path, m_alloc, 16);
            }
        }
        else
        {
            m_alloc = path.m_alloc;
            copy(path.m_path, m_path, m_alloc, 16);
        }
        return *this;
    }

	xpath& xpath::operator+=(const utf32::runes& r)
	{
		concatenate(m_path, r, m_alloc, 16);
		return *this;
	}

    bool xpath::operator==(const xpath& rhs) const { return compare(m_path, rhs.m_path) == 0; }
    bool xpath::operator!=(const xpath& rhs) const { return compare(m_path, rhs.m_path) != 0; }

	void xpath::append_utf16(xpath const& fp, utf16::crunes const& r)
	{
        utf16::pcrune src = (utf16::pcrune)r.m_str;
        utf32::prune  dst = (utf32::prune)fp.m_path.m_str;
        while (src < r.m_end)
        {
            uchar32 c = utf::read(src, r.m_end);
            utf::write(c, dst, fp.m_path.m_end);
        }
	}

    void xpath::view_utf16(xpath const& fp, utf16::crunes& runes) 
    {
        utf16::prune str  = (utf16::prune)fp.m_path.m_str;
        utf16::prune end  = (utf16::prune)fp.m_path.m_end;
        utf16::prune eos  = (utf16::prune)fp.m_path.m_eos;
        utf32::pcrune src = (utf32::pcrune)fp.m_path.m_str;
        while (src <= fp.m_path.m_end)
        {
            uchar32 c = *src++;
            utf::write(c, end, eos);
        }
		runes.m_str = (utf16::prune)fp.m_path.m_str;
		runes.m_end = (utf16::prune)end;
		runes.m_cur = runes.m_str;
    }

    void xpath::release_utf16(xpath const& fp, utf16::crunes& runes) 
    {
        utf16::pcrune src = (utf16::pcrune)runes.m_str;
        utf32::prune  dst = (utf32::prune)fp.m_path.m_str;
        while (src < runes.m_end)
        {
            uchar32 c = utf::read(src, runes.m_end);
            utf::write(c, dst, fp.m_path.m_end);
        }
    }

	void xpath::append_utf16(xfilepath const& fp, utf16::crunes const& r)
	{
		append_utf16(fp.mPath, r);
	}

    void xpath::view_utf16(xfilepath const& fp, utf16::crunes& runes) 
    {
		view_utf16(fp.mPath, runes);
    }

    void xpath::release_utf16(xfilepath const& fp, utf16::crunes& runes) 
    {
		release_utf16(fp.mPath, runes);
    }


	void xpath::append_utf16(xdirpath const& fp, utf16::crunes const& r)
	{
		append_utf16(fp.mPath, r);
	}

    void xpath::view_utf16(xdirpath const& fp, utf16::crunes& runes) 
    {
		view_utf16(fp.mPath, runes);
    }

    void xpath::release_utf16(xdirpath const& fp, utf16::crunes& runes) 
    {
		release_utf16(fp.mPath, runes);
    }

} // namespace xcore
