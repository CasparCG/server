#pragma once

#include <vector>
#include "lockable.h"

namespace caspar {
namespace utils {

// TODO: (R.N) Create a scalable allocator with seperate memory pools for different threads, aka. tbb::scalable_allocator

template<unsigned char BlocksInChunk = 8, unsigned char Alignment = 16>
class FixedAllocator : private utils::LockableObject
{
	struct Chunk
	{
		void Init(std::size_t blockSize, unsigned char blocks);
		void Destroy();

		void* Allocate();
		void Deallocate(void* p);

		unsigned char blocksAvailable_;
		unsigned char* GetDataPtr() const {
			return pData_;
		}
		unsigned char GetAlignmentOffset() const {
			return alignmentOffset_;
		}

	private:
		unsigned char alignmentOffset_;
		unsigned char firstAvailableBlock_;
		unsigned char* pData_;
		unsigned int blockSize_;
	};
	typedef std::vector<Chunk> Chunks;

	FixedAllocator(const FixedAllocator&);
	FixedAllocator& operator=(const FixedAllocator&);
public:
	explicit FixedAllocator(unsigned int blockSize);

	~FixedAllocator();

	void* Allocate();
	void Deallocate(void* p);

private:
	bool IsPtrInChunk(void* p, const Chunk& c) {
		return (p >= c.GetDataPtr() && p < (c.GetDataPtr() + chunkSize + c.GetAlignmentOffset()));
	}

	unsigned int blockSize_;
	const std::size_t chunkSize;
	Chunks chunks_;

	Chunk* pAllocChunk_;
	Chunk* pDeallocChunk_;
};
typedef std::tr1::shared_ptr<FixedAllocator<> > FixedAllocatorPtr;

template<unsigned char BlocksInChunk, unsigned char Alignment>
void FixedAllocator<BlocksInChunk, Alignment>::Chunk::Init(std::size_t blockSize, unsigned char blocks) {
	pData_ = new unsigned char[blockSize*blocks + Alignment];
	alignmentOffset_ = static_cast<unsigned char>(Alignment - (reinterpret_cast<std::size_t>(pData_) % Alignment));
	firstAvailableBlock_ = 0;
	blocksAvailable_ = blocks;
	blockSize_ = blockSize;

	unsigned char i = 0;
	unsigned char* p = pData_ + alignmentOffset_;
	while(i < blocks) {
		*p = ++i;
		p += blockSize;
	}
}
template<unsigned char BlocksInChunk, unsigned char Alignment>
void FixedAllocator<BlocksInChunk, Alignment>::Chunk::Destroy() {
	if(pData_ != 0) {
		delete[] pData_;
		pData_ = 0;
		blocksAvailable_ = 0;
		firstAvailableBlock_ = 0;
		alignmentOffset_ = 0;
		blockSize_ = 0;
	}	
}

template<unsigned char BlocksInChunk, unsigned char Alignment>
void* FixedAllocator<BlocksInChunk, Alignment>::Chunk::Allocate() {
	if(blocksAvailable_ < 1) return 0;

	unsigned char* pResult = pData_ + (firstAvailableBlock_ * blockSize_ + alignmentOffset_);
	firstAvailableBlock_ = *pResult;
	--blocksAvailable_;

	return pResult;
}

template<unsigned char BlocksInChunk, unsigned char Alignment>
void FixedAllocator<BlocksInChunk, Alignment>::Chunk::Deallocate(void* p) {
	_ASSERT(p >= pData_);

	unsigned char* pToRelease = static_cast<unsigned char*>(p);
	_ASSERT((pToRelease - pData_) % blockSize_ == alignmentOffset_);
	*pToRelease = firstAvailableBlock_;

	firstAvailableBlock_ = static_cast<unsigned char>((pToRelease - pData_) / blockSize_);
	_ASSERT(firstAvailableBlock_ == (pToRelease - pData_) / blockSize_);

	++blocksAvailable_;
}


template<unsigned char BlocksInChunk, unsigned char Alignment>
FixedAllocator<BlocksInChunk, Alignment>::FixedAllocator(unsigned int blockSize) : blockSize_(blockSize), chunkSize(blockSize*BlocksInChunk), pAllocChunk_(0), pDeallocChunk_(0) {
}

template<unsigned char BlocksInChunk, unsigned char Alignment>
FixedAllocator<BlocksInChunk, Alignment>::~FixedAllocator() {
	Chunks::iterator it = chunks_.begin();
	Chunks::iterator end = chunks_.end();

	for(; it != end; ++it) {
		(*it).Destroy();
	}

	chunks_.clear();
}

template<unsigned char BlocksInChunk, unsigned char Alignment>
void* FixedAllocator<BlocksInChunk, Alignment>::Allocate() {
	Lock lock(*this);

	if(pAllocChunk_ == 0 || pAllocChunk_->blocksAvailable_ == 0) {
		Chunks::iterator it = chunks_.begin();
		Chunks::iterator end = chunks_.end();
		for(;; ++it) {
			if(it == end) {
				Chunk newChunk;
				newChunk.Init(blockSize_, BlocksInChunk);
				chunks_.push_back(newChunk);
				pAllocChunk_ = &chunks_.back();
				pDeallocChunk_ = &chunks_.back();
				break;
			}

			if(it->blocksAvailable_ > 0) {
				pAllocChunk_ = &(*it);
				break;
			}
		}
	}

	_ASSERT(pAllocChunk_ != 0);
	_ASSERT(pAllocChunk_->blocksAvailable_ > 0);
	return pAllocChunk_->Allocate();
}

template<unsigned char BlocksInChunk, unsigned char Alignment>
void FixedAllocator<BlocksInChunk, Alignment>::Deallocate(void *p) {
	Lock lock(*this);

	_ASSERT(pDeallocChunk_ != 0);

	Chunks::iterator end = chunks_.end();

	if(IsPtrInChunk(p, *pDeallocChunk_)) {
		pDeallocChunk_->Deallocate(p);
	}
	else {
		Chunks::iterator it = chunks_.begin();
		for(; it != end; ++it) {
			if(IsPtrInChunk(p, (*it))) {
				(*it).Deallocate(p);
				pDeallocChunk_ = &(*it);
				break;
			}
		}
		_ASSERT(it != end);
	}
	
	//If this deallocation emptied the chunk, move it to the end
	if(pDeallocChunk_->blocksAvailable_ == BlocksInChunk) {
		--end;
		//destroy the currently last chunk if the that also is empty
		if(&(*end) != pDeallocChunk_ && end->blocksAvailable_ == BlocksInChunk) {
			end->Destroy();
			chunks_.erase(end);
		}

		std::swap(*pDeallocChunk_, chunks_.back());

//		ASSERT(pDeallocChunk_->blocksAvailable_ > 0);
	}
}

}	//namespace utils
}	//namespace caspar