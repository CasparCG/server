#pragma once

#include <string>
#include <vector>

#include "ciicommand.h"

namespace caspar {

namespace cii {

class CIIProtocolStrategy;

class MediaCommand : public ICIICommand
{
public:
	MediaCommand(CIIProtocolStrategy* pPS) : pCIIStrategy_(pPS)
	{}

	virtual int GetMinimumParameters() {
		return 1;
	}

	virtual void Setup(const std::vector<tstring>& parameters);
	virtual void Execute();

private:
	tstring graphicProfile_;

	CIIProtocolStrategy* pCIIStrategy_;
};

class WriteCommand : public ICIICommand
{
public:
	WriteCommand(CIIProtocolStrategy* pPS) : pCIIStrategy_(pPS)
	{}

	virtual int GetMinimumParameters() {
		return 2;
	}

	virtual void Setup(const std::vector<tstring>& parameters);
	virtual void Execute();

private:
	tstring targetName_;
	tstring templateName_;
	tstring xmlData_;

	CIIProtocolStrategy* pCIIStrategy_;
};

class MiscellaneousCommand : public ICIICommand
{
public:
	MiscellaneousCommand(CIIProtocolStrategy* pPS) : pCIIStrategy_(pPS), state_(-1), layer_(0)
	{}

	virtual int GetMinimumParameters() {
		return 5;
	}

	virtual void Setup(const std::vector<tstring>& parameters);
	virtual void Execute();

private:
	tstring filename_;
	tstring xmlData_;
	int state_;
	int layer_;

	CIIProtocolStrategy* pCIIStrategy_;
};

class ImagestoreCommand : public ICIICommand
{
public:
	ImagestoreCommand(CIIProtocolStrategy* pPS) : pCIIStrategy_(pPS)
	{}

	virtual int GetMinimumParameters() {
		return 1;
	}

	virtual void Setup(const std::vector<tstring>& parameters);
	virtual void Execute();

private:
	tstring titleName_;

	CIIProtocolStrategy* pCIIStrategy_;
};

class KeydataCommand : public ICIICommand
{
public:
	KeydataCommand(CIIProtocolStrategy* pPS) : pCIIStrategy_(pPS), state_(-1), layer_(0)
	{}

	virtual int GetMinimumParameters() {
		return 1;
	}

	virtual void Setup(const std::vector<tstring>& parameters);
	virtual void Execute();

private:
	tstring titleName_;
	int state_;
	int layer_;

	CIIProtocolStrategy* pCIIStrategy_;
};

}	//namespace cii
}	//namespace caspar