
#pragma once

#include <stdint.h>
#include <vector>
using namespace std;

#ifdef Win64
#define  ISWin64
#endif // Win64


#pragma pack(push, 4)

typedef struct __MMBlock{
	uint32_t mUsedLen;
	uint32_t mNextBlock;
 	void* pContent;
}MMBlock;

typedef struct __MMPack{
	uint32_t mDataSize;
	uint32_t mFirstBlock;
}MMPack;

#define __BlockSize sizeof(MMBlock)
#define __PackSize  sizeof(MMPack)

#pragma pack(pop)

#define __InitBlockNum			64
#define __InitPackNum			64
#define __InitBlockBufferSize	0x1000
typedef int32_t MMCacheBuffer;

class CMMCache
{
public:
	CMMCache(uint32_t BMmemSize = __InitBlockBufferSize) : m_BMmemSize(BMmemSize), 
		m_PackMaxNum(__InitPackNum), 
		m_BlockMaxNum(__InitBlockNum){
		
		m_pPackList = (MMPack*)malloc(__PackSize*m_PackMaxNum);
		memset(m_pPackList, 0, __PackSize*m_PackMaxNum);
		for (uint32_t pidx = 0; pidx < m_PackMaxNum; pidx){
			mFreePacks.push_back(pidx);
		}
		
		m_pBlockList = (MMBlock*)malloc(__BlockSize*m_BlockMaxNum);
		memset(m_pBlockList, 0, __BlockSize*m_BlockMaxNum);
		for (uint32_t bidx = 0; bidx < m_BlockMaxNum; bidx){
			mFreeBlocks.push_back(bidx);
		}
	}

	~CMMCache(){
		MMBlock* ptrBlock = nullptr;
		for (uint32_t idx = 0; idx < mFreeBlocks.size(); idx++)
		{
			ptrBlock = m_pBlockList + idx;
			if (ptrBlock->pContent != nullptr)
			{
				free(ptrBlock->pContent);
			}
		}
		free(m_pBlockList);
		free(m_pPackList);
	}

	MMCacheBuffer RequestMem(int32_t size){
		if (size <= 0){
			return -1;
		}
		MMCacheBuffer PackId = PopPack();
		uint32_t blockId = PopBlock();
		MMPack* pCurPack = m_pPackList + PackId;
		pCurPack->mDataSize = size;
		pCurPack->mFirstBlock = blockId;
		MMBlock* pCurBlock = m_pBlockList + blockId;
		size -= __InitBlockBufferSize;

		while (size > 0)
		{
			uint32_t NewBlkId = PopBlock();
			pCurBlock->mNextBlock = NewBlkId;
			pCurBlock = m_pBlockList + NewBlkId;
			size -= __InitBlockBufferSize;
		}

		return PackId;
	}
	bool ReleaseMem(MMCacheBuffer buffer){
		if (buffer < 0 || buffer >= m_PackMaxNum){
			return false;
		}
		MMPack* lsPack = m_pPackList + buffer;
		uint32_t BkId = lsPack->mFirstBlock;
		while (BkId  > 0)
		{
			mFreeBlocks.push_back(BkId);
			MMBlock* lsBlock = m_pBlockList + BkId;
			BkId = lsBlock->mNextBlock;
		}
		mFreePacks.push_back(buffer);
		return true;
	}

	uint32_t Write(MMCacheBuffer Dest, uint32_t DestOffset, void* Src, uint32_t Size){
		if (Dest >= m_PackMaxNum || Dest < 0){
			return 0;
		}
		MMPack* pPack = m_pPackList + Dest;
		if (pPack->mDataSize < Size){
			return 0;
		}
		uint32_t nBlockId = pPack->mFirstBlock;
		if (nBlockId >= m_BlockMaxNum || nBlockId < 0){
			return 0;
		}
		MMBlock* pBlock = m_pBlockList + pPack->mFirstBlock;
		if (pBlock->pContent == nullptr){
			return 0;
		}
	}

private:
	uint32_t PopPack(){
		if (mFreePacks.size() == 0)
		{
			uint32_t NewPackMaxNum = m_PackMaxNum + __InitPackNum;
			MMPack*  NewPackList = (MMPack*)malloc(NewPackMaxNum*__PackSize);
			memset(NewPackList, 0, NewPackMaxNum*__PackSize);
			memcpy(NewPackList, m_pPackList, m_PackMaxNum*__PackSize);
			free(m_pPackList);			
			for (uint32_t NewId = m_PackMaxNum; NewId < NewPackMaxNum; NewId++)
			{
				mFreePacks.push_back(NewId);
			}

			m_PackMaxNum = NewPackMaxNum;
			m_pPackList = NewPackList;
		}
		uint32_t PackId = mFreePacks.at(mFreePacks.size() - 1);
		mFreePacks.pop_back();
		return PackId;
	}

	uint32_t PopBlock(){
		if (mFreeBlocks.size() == 0)
		{
			uint32_t NewBlockMaxNum = m_BlockMaxNum + __InitBlockNum;
			MMBlock* NewBlockList = (MMBlock*)malloc(NewBlockMaxNum* __BlockSize);
			memset(NewBlockList, 0, NewBlockMaxNum*__BlockSize);
			memcpy(NewBlockList, m_pBlockList, m_BlockMaxNum*__BlockSize);
			free(m_pBlockList);
			for (uint32_t NewId = m_BlockMaxNum; NewId < NewBlockMaxNum; NewId++)
			{
				mFreeBlocks.push_back(NewId);
			}

			m_BlockMaxNum = NewBlockMaxNum;
			m_pBlockList = NewBlockList;
		}
		uint32_t BlockId = mFreeBlocks.at(mFreeBlocks.size() - 1);
		mFreeBlocks.pop_back();
		return BlockId;
	}

protected:
	const uint32_t	m_BMmemSize;

	MMBlock*			m_pBlockList;
	vector<uint32_t>	mFreeBlocks;
	uint32_t			m_BlockMaxNum;
	
	MMPack*				m_pPackList;
	vector<uint32_t>	mFreePacks;
	uint32_t			m_PackMaxNum;

};



/*

CMM
private:
WorkedBlocked, FreeBolcked
CPack
block index array
CBlock

pack:
PackPtrMem			|Pack*|Pack*|Pack*|......
BlockPtrMem			|Block*|Block*|Block*|......

func 1:	init: malloc n blocks
func 2: request: (size) ->
1. malloc 1 pack
2. calculate need block num
3. find free blocks or malloc new block
4. return pack ptr
func 3: release: (pack*)->
1. locate pack*
2. set relative block free
3. set pack free
func 4: write: (pack*, offset, src*, size)->
1. locate pack*
2. locate block*
3. calculate offset & size
4. write to mem
5. return write size
fucn 5: read: (pack*, offset, src*, size)->
1. locate pack*
2. locate block*
3. calculate offset & size
4. write to src
5. return read size


0x1000;		// 4k		4096 bytes
0x10000;	// 64k		65536 bytes
0x100000;	// 1024k	1048576 bytes

*/



