//==============================================================================
// INCLUDES
//==============================================================================
#include "xbase\x_target.h"
#include "xbase\x_debug.h"
#include "xbase\x_string_std.h"
#include "xbase\x_va_list.h"
#include "xbase\x_bit_field.h"

#include "xfilesystem\x_filesystem.h"
#include "xfilesystem\x_filestream.h"
#include "xfilesystem\x_filepath.h"
#include "xfilesystem\x_stream.h"
#include "xfilesystem\x_istream.h"

#include "xfilesystem\private\x_filesystem_common.h"
#include "xfilesystem\private\x_filesystem_private.h"

//==============================================================================
// xcore namespace
//==============================================================================
namespace xcore
{
	namespace xfilesystem
	{
		//------------------------------------------------------------------------------------------
		
		class xasync_result_ctor : public xasync_result
		{
		public:
			xasync_result_ctor(xiasync_result* imp) : xasync_result(imp) { }
		};

		//------------------------------------------------------------------------------------------

		class xifilestream;

		static xifilestream*	sConstructFileStream(const xfilepath& filename, EFileMode mode, EFileAccess access, EFileOp op);
		static void				sDestructFileStream(xifilestream* stream);

		class xifilestream : public xistream
		{
			s32						mRefCount;
			u32						mFileHandle;
			s64						mFileSeekPos;

			enum ECaps
			{
				NONE		= 0x0000,
				CAN_READ	= 0x0001,
				CAN_SEEK	= 0x0002,
				CAN_WRITE	= 0x0004,
				CAN_ASYNC	= 0x0008,
				USE_READ	= 0x1000,
				USE_SEEK	= 0x2000,
				USE_WRITE	= 0x4000,
				USE_ASYNC	= 0x8000,
			};
			x_bitfield<ECaps>		mCaps;

		public:
									xifilestream(const xfilepath& filename, EFileMode mode, EFileAccess access, EFileOp op);
									~xifilestream(void);

			virtual void			hold()																{ mRefCount++; }
			virtual s32				release()															{ --mRefCount; return mRefCount; }
			virtual void			destroy()															{ close(); sDestructFileStream(this); }

			virtual bool			canRead() const;													///< Gets a value indicating whether the current stream supports reading. (Overrides xstream.CanRead.)
			virtual bool			canSeek() const;													///< Gets a value indicating whether the current stream supports seeking. (Overrides xstream.CanSeek.)
			virtual bool			canWrite() const;													///< Gets a value indicating whether the current stream supports writing. (Overrides xstream.CanWrite.)
			virtual bool			isAsync() const;													///< Gets a value indicating whether the stream was opened asynchronously or synchronously.
			virtual u64				length() const;														///< Gets the length in bytes of the stream. (Overrides xstream.Length.)
			virtual u64				position() const;													///< Gets the current position of this stream. (Overrides xstream.Position.)
			virtual void			position(u64 Pos);													///< Sets the current position of this stream. (Overrides xstream.Position.)

			virtual s64				seek(s64 offset, ESeekOrigin origin);			 					///< When overridden in a derived class, sets the position within the current stream.
			virtual void			close(); 															///< Closes the current stream and releases any resources (such as sockets and file handles) associated with the current stream.
			virtual void			flush();															///< When overridden in a derived class, clears all buffers for this stream and causes any buffered data to be written to the underlying device.

			virtual void			setLength(s64 length);								 				///< When overridden in a derived class, sets the length of the current stream.

			virtual void			read(xbyte* buffer, s32 offset, s32 count);		 					///< When overridden in a derived class, reads a sequence of bytes from the current stream and advances the position within the stream by the number of bytes read.
			virtual s32				readByte();											 				///< Reads a byte from the stream and advances the position within the stream by one byte, or returns -1 if at the end of the stream.
			virtual void			write(const xbyte* buffer, s32 offset, s32 count);					///< When overridden in a derived class, writes a sequence of bytes to the current stream and advances the current position within this stream by the number of bytes written.
			virtual void			writeByte(xbyte value);								 				///< Writes a byte to the current position in the stream and advances the position within the stream by one byte.

			virtual xasync_result	beginRead(xbyte* buffer, s32 offset, s32 count, AsyncCallback callback);	  	///< Begins an asynchronous read operation.
			virtual void			endRead(xasync_result& asyncResult);											///< Waits for the pending asynchronous read to complete.
			virtual xasync_result	beginWrite(const xbyte* buffer, s32 offset, s32 count, AsyncCallback callback);	///< Begins an asynchronous write operation.
			virtual void			endWrite(xasync_result& asyncResult);											///< Ends an asynchronous write operation.

			virtual void			copyTo(xistream* dst);												///< Reads the bytes from the current stream and writes them to the destination stream.
			virtual void			copyTo(xistream* dst, s32 count);									///< Reads all the bytes from the current stream and writes them to a destination stream, using a specified buffer size.

			XFILESYSTEM_OBJECT_NEW_DELETE()
		};

		static xifilestream*	sConstructFileStream(const xfilepath& filename, EFileMode mode, EFileAccess access, EFileOp op)
		{
			xifilestream* fs = new xifilestream(filename, mode, access, op);
			return fs;
		}
		static void				sDestructFileStream(xifilestream* stream)
		{
			delete stream;
		}

		// -------------------------- xfilestream --------------------------

		xfilestream::xfilestream(const xfilestream& other)
			: xstream(other.mImplementation)
		{
		}

		xfilestream::xfilestream(const xfilepath& filename, EFileMode mode, EFileAccess access, EFileOp op)
		{
			mImplementation = sConstructFileStream(filename, mode, access, op);
		}

		xfilestream::~xfilestream()
		{
		}

		xfilestream&			xfilestream::operator =			(const xfilestream& other)
		{
			if (mImplementation->release() == 0)
				mImplementation->destroy();
			mImplementation = other.mImplementation;
			mImplementation->hold();
			return *this;
		}

		bool					xfilestream::operator ==		(const xfilestream& other) const
		{
			return other.mImplementation == mImplementation;
		}

		bool					xfilestream::operator !=		(const xfilestream& other) const
		{
			return other.mImplementation != mImplementation;
		}

		// ---------------------------------------------------------------------------------------------

		xifilestream::xifilestream(const xfilepath& filename, EFileMode mode, EFileAccess access, EFileOp op)
			: mRefCount(1)
			, mFileHandle(INVALID_FILE_HANDLE)
			, mFileSeekPos(0)
			, mCaps(NONE)
		{
			bool can_read, can_write, can_seek, can_async;
			xfilesystem::caps(filename, can_read, can_write, can_seek, can_async);
			mCaps.set(CAN_WRITE, can_write);
			mCaps.set(CAN_READ, can_read);
			mCaps.set(CAN_SEEK, can_seek);
			mCaps.set(CAN_ASYNC, can_async);

			mCaps.set(USE_READ, true);
			mCaps.set(USE_SEEK, can_seek);
			mCaps.set(USE_WRITE, can_write && ((access&FileAccess_Write)!=0));
			mCaps.set(USE_ASYNC, can_async && (op == FileOp_Async));

			switch(mode)
			{
				case FileMode_CreateNew:
					{
						if (mCaps.isSet(CAN_WRITE))
						{
							if (xfilesystem::doesFileExist(filename.c_str()) == xFALSE)
							{
								mFileHandle = xfilesystem::open(filename.c_str(), mCaps.isSet(USE_READ), mCaps.isSet(USE_WRITE), mCaps.isSet(USE_ASYNC));
							}
						}
					} break;
				case FileMode_Create:
					{
						if (mCaps.isSet(CAN_WRITE))
						{
							if (xfilesystem::doesFileExist(filename.c_str()) == xTRUE)
							{
								mFileHandle = xfilesystem::open(filename.c_str(), mCaps.isSet(USE_READ), mCaps.isSet(USE_WRITE), mCaps.isSet(USE_ASYNC));
								xfilesystem::reSize(mFileHandle, 0);
							}
							else
							{
								mFileHandle = xfilesystem::open(filename.c_str(), mCaps.isSet(USE_READ), mCaps.isSet(USE_WRITE), mCaps.isSet(USE_ASYNC));
							}
						}
					} break;
				case FileMode_Open:
					{
						if (xfilesystem::doesFileExist(filename.c_str()) == xTRUE)
						{
							mFileHandle = xfilesystem::open(filename.c_str(), mCaps.isSet(USE_READ), mCaps.isSet(USE_WRITE), mCaps.isSet(USE_ASYNC));
						}
					} break;
				case FileMode_OpenOrCreate:
					{
						mFileHandle = xfilesystem::open(filename.c_str(), mCaps.isSet(USE_READ), mCaps.isSet(USE_WRITE), mCaps.isSet(USE_ASYNC));
					} break;
				case FileMode_Truncate:
					{
						if (mCaps.isSet(CAN_WRITE))
						{
							if (xfilesystem::doesFileExist(filename.c_str()) == xTRUE)
							{
								mFileHandle = xfilesystem::open(filename.c_str(), mCaps.isSet(USE_READ), mCaps.isSet(USE_WRITE), mCaps.isSet(USE_ASYNC));
								if (mFileHandle != INVALID_FILE_HANDLE)
								{
									xfilesystem::reSize(mFileHandle, 0);
								}
							}
						}
					} break;
				case FileMode_Append:
					{
						if (mCaps.isSet(CAN_WRITE))
						{
							if (xfilesystem::doesFileExist(filename.c_str()) == xTRUE)
							{
								mFileHandle = xfilesystem::open(filename.c_str(), mCaps.isSet(USE_READ), mCaps.isSet(USE_WRITE), mCaps.isSet(USE_ASYNC));
								if (mFileHandle != INVALID_FILE_HANDLE)
								{
									mCaps.set(USE_READ, false);
									mCaps.set(USE_SEEK, false);
									mCaps.set(USE_WRITE, true);

									mFileSeekPos = xfilesystem::size(mFileHandle);
								}
							}
						}
					} break;
			}
		}

		xifilestream::~xifilestream(void)
		{
		}

		bool			xifilestream::canRead() const
		{
			return mCaps.isSet(CAN_READ);
		}

		bool			xifilestream::canSeek() const
		{
			return mCaps.isSet(CAN_SEEK);
		}

		bool			xifilestream::canWrite() const
		{
			return mCaps.isSet(CAN_WRITE);
		}

		bool			xifilestream::isAsync() const
		{
			return mCaps.isSet(USE_ASYNC);
		}

		u64				xifilestream::length() const
		{
			return xfilesystem::size(mFileHandle);
		}

		u64				xifilestream::position() const
		{
			return (u64)mFileSeekPos;
		}

		void			xifilestream::position(u64 Pos)
		{
			mFileSeekPos = Pos;
		}

		s64				xifilestream::seek(s64 offset, ESeekOrigin origin)
		{
			if (mCaps.isSet(USE_SEEK))
			{
				switch (origin)
				{
					case Seek_Begin: mFileSeekPos = offset; break;
					case Seek_Current: mFileSeekPos += offset; break;
					case Seek_End: mFileSeekPos = (s64)length() + offset; break;
				}
				
				if (mFileSeekPos < 0)
					mFileSeekPos = 0;

				return mFileSeekPos;
			}
			return 0;
		}

		void			xifilestream::close()
		{
			if (mFileHandle != INVALID_FILE_HANDLE)
			{
				xfilesystem::syncClose(mFileHandle);
			}
		}

		void			xifilestream::flush()
		{
		}

		void			xifilestream::setLength(s64 length)
		{
			xfilesystem::reSize(mFileHandle, length);
		}

		void			xifilestream::read(xbyte* buffer, s32 offset, s32 count)
		{
			xfilesystem::syncRead(mFileHandle, mFileSeekPos, count, &buffer[offset]);
		}

		s32				xifilestream::readByte()
		{
			xbyte data[4];
			xfilesystem::syncRead(mFileHandle, mFileSeekPos, 1, data);
			return (s32)data[0];
		}

		void			xifilestream::write(const xbyte* buffer, s32 offset, s32 count)
		{
			xfilesystem::syncWrite(mFileHandle, mFileSeekPos, count, &buffer[offset]);
		}

		void			xifilestream::writeByte(xbyte value)
		{
			xbyte data[4];
			data[0] = value;
			xfilesystem::syncWrite(mFileHandle, mFileSeekPos, 1, data);
		}

		xasync_result	xifilestream::beginRead(xbyte* buffer, s32 offset, s32 count, AsyncCallback callback)
		{
			xiasync_result* async_result_imp;
			xfilesystem::asyncRead(mFileHandle, mFileSeekPos, count, &buffer[offset], async_result_imp);
			return xasync_result_ctor(async_result_imp);
		}

		void			xifilestream::endRead(xasync_result& asyncResult)
		{
			asyncResult.waitUntilCompleted();
		}

		xasync_result	xifilestream::beginWrite(const xbyte* buffer, s32 offset, s32 count, AsyncCallback callback)
		{
			xiasync_result* async_result_imp;
			xfilesystem::asyncWrite(mFileHandle, mFileSeekPos, count, &buffer[offset], async_result_imp);
			return xasync_result_ctor(async_result_imp);
		}

		void			xifilestream::endWrite(xasync_result& asyncResult)
		{
			asyncResult.waitUntilCompleted();
		}

		void			xifilestream::copyTo(xistream* dst)
		{
		}

		void			xifilestream::copyTo(xistream* dst, s32 count)
		{
		}

	};

	//==============================================================================
	// END xcore namespace
	//==============================================================================
};