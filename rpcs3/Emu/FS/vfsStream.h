#pragma once

struct vfsStream
{
	vfsStream() = default;

	virtual ~vfsStream()
	{
		Close();
	}

	virtual void Close()
	{
	}

	virtual u64 GetSize() const = 0;

	virtual u64 Write(const void* src, u64 count) = 0;

	template<typename T>
	force_inline bool SWrite(const T& data, u64 count = sizeof(T))
	{
		return Write(&data, count) == count;
	}

	virtual u64 Read(void* dst, u64 count) = 0;

	template<typename T>
	force_inline bool SRead(T& data, u64 count = sizeof(T))
	{
		return Read(&data, count) == count;
	}

	template<typename T>
	bool VWrite(const std::vector<T>& vec)
	{
		return IsOpened() && Write(vec.data(), vec.size() * sizeof(T)) == vec.size() * sizeof(T);
	}

	template<typename T>
	std::vector<T> VRead()
	{
		std::vector<T> result;
		if (IsOpened() == false)
		{
			return result;
		}

		result.resize(GetSize() / sizeof(T));

		if (Seek(0) == -1 || Read(result.data(), result.size() * sizeof(T)) != result.size() * sizeof(T))
		{
			result.clear();
		}

		return result;
	}

	virtual u64 Seek(s64 offset, fs::seek_mode whence = fs::seek_set) = 0;

	virtual u64 Tell() const = 0;

	virtual bool Eof() const
	{
		return Tell() >= GetSize();
	}

	virtual bool IsOpened() const = 0;
};
