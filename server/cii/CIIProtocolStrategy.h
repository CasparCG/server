#pragma once
#include <vector>
#include "..\io\ProtocolStrategy.h"
#include "..\utils\commandqueue.h"
#include "..\channel.h"
#include "..\MediaManager.h"
#include "CIICommand.h"

namespace caspar {
	namespace CG { class ICGControl; }
namespace cii {

class CIIProtocolStrategy : public caspar::IO::IProtocolStrategy
{
public:
	CIIProtocolStrategy();
	virtual ~CIIProtocolStrategy();

	virtual void Parse(const TCHAR* pData, int charCount, caspar::IO::ClientInfoPtr pClientInfo);

	void SetProfile(const tstring& profile) {
		currentProfile_ = profile;
	}
	caspar::CG::ICGControl* GetCGControl() const {
		return pCGControl_;
	}
	void DisplayMediaFile(const tstring& filename);
	void DisplayTemplate(const tstring& titleName);
	void WriteTemplateData(const tstring& templateName, const tstring& titleName, const tstring& xmlData);

public:
	struct TitleHolder
	{
		TitleHolder() : titleName(TEXT(""))
		{}
		TitleHolder(const tstring& name, MediaProducerPtr pFP) : titleName(name), pMediaProducer(pFP)
		{}
		TitleHolder(const TitleHolder& th) : titleName(th.titleName), pMediaProducer(th.pMediaProducer)
		{}
		const TitleHolder& operator=(const TitleHolder& th) {
			titleName = th.titleName;
			pMediaProducer = th.pMediaProducer;
		}
		bool operator==(const TitleHolder& rhs) {
			return pMediaProducer == rhs.pMediaProducer;
		}

		tstring titleName;
		MediaProducerPtr pMediaProducer;
		friend CIIProtocolStrategy;
	};
private:
	friend TitleHolder;
	friend bool operator==(const TitleHolder& lhs, const tstring& rhs);
	friend bool operator==(const tstring& lhs, const TitleHolder& rhs);

	typedef std::list<TitleHolder> TitleList;
	TitleList titles_;
	MediaProducerPtr GetPreparedTemplate(const tstring& name);
	void PutPreparedTemplate(const tstring& name, MediaProducerPtr& pMediaProducer);

	static const TCHAR TokenDelimiter;
	static const tstring MessageDelimiter;

	void ProcessMessage(const tstring& message);
	int TokenizeMessage(const tstring& message, std::vector<tstring>* pTokenVector);
	CIICommandPtr Create(const tstring& name);

	caspar::utils::CommandQueue<CIICommandPtr> commandQueue_;
	tstring currentMessage_;

	tstring currentProfile_;
	caspar::MediaManagerPtr pFlashManager_;
	caspar::CG::ICGControl* pCGControl_;
	ChannelPtr pChannel_;
};

}	//namespace cii
}	//namespace caspar
