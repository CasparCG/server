/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
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