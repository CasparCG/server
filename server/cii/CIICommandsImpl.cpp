#include "..\StdAfx.h"

#include "CIIProtocolStrategy.h"
#include "..\channel.h"
#include "..\cg\cgcontrol.h"
#include "CIICommandsImpl.h"
#include <sstream>

namespace caspar {
namespace cii {

/////////////////
// MediaCommand
void MediaCommand::Setup(const std::vector<tstring>& parameters) {
	graphicProfile_ = parameters[1].substr(2);
}

void MediaCommand::Execute() {
	pCIIStrategy_->SetProfile(graphicProfile_);
}


/////////////////
// WriteCommand
void WriteCommand::Setup(const std::vector<tstring>& parameters) {
	try {
		if(parameters.size()>2) {
			targetName_ = parameters[1];
			templateName_ = parameters[2];

			tstringstream dataStream;

			dataStream << TEXT("<templateData>");

			std::vector<tstring>::size_type end = parameters.size();
			for(std::vector<tstring>::size_type i = 3; i < end; ++i) {
				dataStream << TEXT("<componentData id=\"field") << i-2 << TEXT("\"><data id=\"text\" value=\"") << parameters[i] << TEXT("\" /></componentData>"); 
			}

			dataStream << TEXT("</templateData>");
			xmlData_ = dataStream.str();
		}
	}
	catch(std::exception) {
	}
}

void WriteCommand::Execute() {
	pCIIStrategy_->WriteTemplateData(templateName_, targetName_, xmlData_);
}


//////////////////////
// ImagestoreCommand
void ImagestoreCommand::Setup(const std::vector<tstring>& parameters) {
	if(parameters[1] == TEXT("7") && parameters.size() > 2) {
		titleName_ = parameters[2].substr(0, 4);
	}
}

void ImagestoreCommand::Execute() {
	pCIIStrategy_->DisplayTemplate(titleName_);
}


//////////////////////
// MiscellaneousCommand
void MiscellaneousCommand::Setup(const std::vector<tstring>& parameters) {
	//HAWRYS:	V\5\3\1\1\namn.tga\1
	//			Display still
	if((parameters.size() > 5) && parameters[1] == TEXT("5") && parameters[2] == TEXT("3")) {
		filename_ = parameters[5];
		filename_ = filename_.substr(0, filename_.find_last_of(TEXT('.')));
		state_ = 0;
		return;
	}
	
	//NEPTUNE:	V\5\13\1\X\Template\0\TabField1\TabField2...
	//			Add Template to layer X in the active templatehost
	if((parameters.size() > 5) && parameters[1] == TEXT("5") && parameters[2] == TEXT("13"))
	{
		layer_ = _ttoi(parameters[4].c_str());
		filename_ = parameters[5];
		filename_.append(TEXT(".ft"));
		state_ = 1;
		if(parameters.size() > 7) {
			tstringstream dataStream;

			dataStream << TEXT("<templateData>");
			std::vector<tstring>::size_type end = parameters.size();
			for(std::vector<tstring>::size_type i = 7; i < end; ++i) {
				dataStream << TEXT("<componentData id=\"f") << i-7 << TEXT("\"><data id=\"text\" value=\"") << parameters[i] << TEXT("\" /></componentData>"); 
			}
			dataStream << TEXT("</templateData>");

			xmlData_ = dataStream.str();
		}
	}
}

void MiscellaneousCommand::Execute() {
	if(state_ == 0)
	{
		pCIIStrategy_->DisplayMediaFile(filename_);
	}

	//TODO: Need to be checked for validity
	else if(state_ == 1)
		pCIIStrategy_->GetCGControl()->Add(layer_, filename_, false, TEXT(""), xmlData_);
}


///////////////////
// KeydataCommand
void KeydataCommand::Execute() {
	if(state_ == 0)
	{
		pCIIStrategy_->DisplayTemplate(titleName_);
	}

	//TODO: Need to be checked for validity
	else if(state_ == 1)
		pCIIStrategy_->GetCGControl()->Stop(layer_, 0);
	else if(state_ == 2)
		pCIIStrategy_->GetCGControl()->Clear();
	else if(state_ == 3)
		pCIIStrategy_->GetCGControl()->Play(layer_);
}

void KeydataCommand::Setup(const std::vector<tstring>& parameters) {
	//HAWRYS:	Y\<205><247><202><196><192><192><200><248>
	//parameter[1] looks like this: "=g:XXXXh" where XXXX is the name that we want
	if(parameters[1].size() > 6) {
		titleName_.resize(4);
		for(int i=0;i<4;++i) {
			if(parameters[1][i+3] < 176) {
				titleName_ = TEXT("");
				break;
			}
			titleName_[i] = parameters[1][i+3]-144;
		}
		state_ = 0;
	}

	if(parameters.size() > 2)
	{
		layer_ = _ttoi(parameters[2].c_str());
	}

	if(parameters[1].at(0) == 27)	//NEPTUNE:	Y\<27>\X			Stop layer X.
		state_ = 1;
	else if(static_cast<unsigned char>(parameters[1].at(0)) == 254)	//NEPTUNE:	Y\<254>			Clear Canvas. 
		state_ = 2;
	else if(static_cast<unsigned char>(parameters[1].at(0)) == 213)	//NEPTUNE:	Y\<213><243>\X	Play layer X. 
		state_ = 3;
}

}	//namespace cii
}	//namespace caspar