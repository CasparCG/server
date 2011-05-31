#pragma once

namespace caspar {
namespace utils {

template<typename T>
class DataBuffer
{
public:
	typedef T DataType;

	DataBuffer() : pData_(0), length_(0), capacity_(0)
	{}
	DataBuffer(const DataBuffer& b) : pData_(new T[b.capacity_]), capacity_(b.capacity_)
	{
		memcpy(pData_, b.pData_, b.capacity_*sizeof(T));
	}
	const DataBuffer& operator=(const DataBuffer& b)
	{
		DataBuffer temp(b);
		swap(temp);
	}
	~DataBuffer() {
		FreeData();
	}

	void Fill(const T* pSrc, unsigned int count, unsigned int offset=0) {
		int neededCapacity = count + offset;
		Realloc(neededCapacity);

		length_ = neededCapacity;
		memcpy(pData_+offset, pSrc, count*sizeof(T));
	}

	T* GetPtr(unsigned int offset=0) {
		return (pData_+offset);
	}
	unsigned int GetCapacity() {
		return capacity_;
	}
	void SetLength(unsigned int len) {
		length_ = len;
	}
	unsigned int GetLength() {
		return length_;
	}

	void Realloc(unsigned int neededCapacity) {
		if(neededCapacity > capacity_) {
			T* pNewData = new T[neededCapacity];
			FreeData();
			pData_ = pNewData;
			capacity_ = neededCapacity;
		}
	}

private:
	void swap(const DataBuffer& b) throw()
	{
		std::swap(pData_, b.pData_);
		std::swap(capacity_, b.capacity_);
	}

	void FreeData() {
		if(pData_ != 0) {
			delete[] pData_;
			pData_ = 0;
		}
		capacity_ = 0;
	}

	T* pData_;
	unsigned int capacity_;
	unsigned int length_;
};

}	//namespace utils
}	//namespace caspar