#include "..\..\stdafx.h"

#include "ffmpegmanager.h"
#include "..\..\frame\FrameManager.h"
#include "..\..\MediaProducerInfo.h"
#include "..\..\frame\FrameMediaController.h"
#include "..\..\utils\databuffer.h"
#include "..\..\utils\thread.h"
#include "..\..\utils\image\image.hpp"
#include "..\..\audio\audiomanager.h"

extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <ffmpeg/avformat.h>
#include <ffmpeg/avcodec.h>
#include <ffmpeg/swscale.h>
}

namespace caspar {
namespace ffmpeg {

using namespace utils;

////////////////////////////////
//  FFMPEGProducer declaration
//
class FFMPEGProducer : public MediaProducer, FrameMediaController, utils::IRunnable, utils::LockableObject
{
public:
	FFMPEGProducer();
	virtual ~FFMPEGProducer();

	bool Load(const tstring& filename);

	//MediaProducer
	virtual IMediaController* QueryController(const tstring&);
	virtual bool GetProducerInfo(MediaProducerInfo* pInfo);

	//FrameMediaController
	virtual bool Initialize(FrameManagerPtr pFrameManager);
	virtual FrameBuffer& GetFrameBuffer() {
		return frameBuffer_;
	}

private:
	virtual void Run(HANDLE stopEvent);
	virtual bool OnUnhandledException(const std::exception& ex) throw();


	utils::Event initializeEvent_;
	utils::Thread worker_;
	tstring	 filename_;

	MotionFrameBuffer frameBuffer_;

	bool FindStreams();

	bool HandleVideoPacket(AVPacket&);
	bool HandleAudioPacket(AVPacket&);

	void CopyFrame(FramePtr pDest, const unsigned char* pSource, int srcWidth, int srcHeight);

	unsigned char*		pFrameBuffer_;
	FramePtr			pCurrentFrame_;
	AVFormatContext*	pFormatContext_;

	SwsContext*			pSwsContext_;

	//Video members
//		std::queue<FramePtr> videoFrameQueue_;

	AVCodecContext*		pVideoCodecContext_;
	AVCodec*			pVideoCodec_;
	int					videoStreamIndex_;
	double				videoFrameRate_;
	bool				bSwapFields_;

	AVFrame*			pDecodeFrame_;
	AVFrame*			pRGBFrame_;

	////Audio members
	std::queue<caspar::audio::AudioDataChunkPtr> audioDataChunkQueue_;

	AVCodecContext*		pAudioCodecContext_;
	AVCodec*			pAudioCodec_;
	int					audioStreamIndex_;
	int					audioFrameSize_;
	int					sourceAudioFrameSize_;
	
	char*				pAudioDecompBuffer_;
	char*				pAlignedAudioDecompAddr_;
	
	caspar::audio::AudioDataChunkPtr	pCurrentAudioDataChunk_;
	int									currentAudioDataChunkOffset_;

	bool bLoop_;
	FrameManagerPtr pFrameManager_;
	FrameManagerPtr pTempFrameManager_;
};
typedef std::tr1::shared_ptr<FFMPEGProducer> FFMPEGProducerPtr;


//////////////////////////////
//  FFMPEGManager definition
//
const int FFMPEGManager::Alignment = 16;
long FFMPEGManager::static_instanceCount = 0;

//four sec of 16 bit stereo 48kHz should be enough
const int FFMPEGManager::AudioDecompBufferSize = 4*48000*4+Alignment;

FFMPEGManager::FFMPEGManager() {
	InterlockedIncrement(&static_instanceCount);
	if(static_instanceCount == 1) {
		Initialize(); 
	}

	_extensions.push_back(TEXT("mpg"));
	_extensions.push_back(TEXT("avi"));
	_extensions.push_back(TEXT("mov"));
	_extensions.push_back(TEXT("dv"));
	_extensions.push_back(TEXT("wav"));
	_extensions.push_back(TEXT("mp3"));
}

FFMPEGManager::~FFMPEGManager() {
	InterlockedDecrement(&static_instanceCount);
	if(static_instanceCount == 0) {
		Dispose();
	}
}

bool FFMPEGManager::Initialize() {
	av_register_all();
	return true;
}

void FFMPEGManager::Dispose() {
}

MediaProducerPtr FFMPEGManager::CreateProducer(const tstring& filename)
{
	FFMPEGProducerPtr result;
	if(filename.length() > 0) {
		result.reset(new FFMPEGProducer());
		if(!result->Load(filename))
			result.reset();
	}

	return result;
}

bool FFMPEGManager::getFileInfo(FileInfo* pFileInfo)
{
	if(pFileInfo != 0) {
		if(pFileInfo->filetype == TEXT("avi") || pFileInfo->filetype == TEXT("mpg") || pFileInfo->filetype == TEXT("mov") || pFileInfo->filetype == TEXT("dv"))
		{
			pFileInfo->length = 0;	//get real length from file
			pFileInfo->type = TEXT("movie");
			pFileInfo->encoding = TEXT("codec");
		}
		else if(pFileInfo->filetype == TEXT("wav") || pFileInfo->filetype == TEXT("mp3")) {
			pFileInfo->length = 0;	//get real length from file
			pFileInfo->type = TEXT("audio");
			pFileInfo->encoding = TEXT("NA");
		}
		return true;
	}
	return false;
}

///////////////////////////////
//  FFMPEGProducer definition
//
FFMPEGProducer::FFMPEGProducer() :	pFormatContext_(0), pVideoCodec_(0), pVideoCodecContext_(0), pFrameBuffer_(0), 
									bLoop_(false), pSwsContext_(0),
									videoStreamIndex_(-1), pAudioCodecContext_(0), pAudioCodec_(0), audioStreamIndex_(-1), 
									audioFrameSize_(0), sourceAudioFrameSize_(0),
									pDecodeFrame_(0), pRGBFrame_(0), videoFrameRate_(25), bSwapFields_(false),
									initializeEvent_(FALSE, FALSE), frameBuffer_(10),
									pAudioDecompBuffer_(0), pAlignedAudioDecompAddr_(0),
									currentAudioDataChunkOffset_(0)
{
	pAudioDecompBuffer_ = new char[FFMPEGManager::AudioDecompBufferSize];
	int alignmentOffset_ = static_cast<unsigned char>(FFMPEGManager::Alignment - (reinterpret_cast<std::size_t>(pAudioDecompBuffer_) % FFMPEGManager::Alignment));
	pAlignedAudioDecompAddr_ = pAudioDecompBuffer_ + alignmentOffset_;
}

FFMPEGProducer::~FFMPEGProducer() {
	worker_.Stop();

	pCurrentFrame_.reset();

	if(pAudioDecompBuffer_ != 0) {
		delete[] pAudioDecompBuffer_;
		pAudioDecompBuffer_ = 0;
		pAlignedAudioDecompAddr_ = 0;
	}

	if(pFrameBuffer_ != 0) {
		delete[] pFrameBuffer_;
		pFrameBuffer_ = 0;
	}

	if(pSwsContext_ != 0) {
		sws_freeContext(pSwsContext_);
		pSwsContext_ = 0;
	}

	if(pVideoCodec_ != 0) {
		avcodec_close(pVideoCodecContext_);
		pVideoCodec_ = 0;
	}
	if(pRGBFrame_ != 0) {
		av_free(pRGBFrame_);
		pRGBFrame_ = 0;
	}
	if(pDecodeFrame_ != 0) {
		av_free(pDecodeFrame_);
		pDecodeFrame_ = 0;
	}

	if(pAudioCodec_ != 0) {
		avcodec_close(pAudioCodecContext_);
		pAudioCodec_ = 0;
	}

	if(pFormatContext_ != 0) {
		av_close_input_file(pFormatContext_);
		pFormatContext_ = 0;
	}
	pVideoCodecContext_ = 0;
	pAudioCodecContext_ = 0;
}

bool ConvertWideCharToLatin1(const std::wstring& wideString, caspar::utils::DataBuffer<char>& destBuffer)
{
	//28591 = ISO 8859-1 Latin I
	int bytesWritten = 0;
	int multibyteBufferCapacity = WideCharToMultiByte(28591, 0, wideString.c_str(), -1, 0, 0, NULL, NULL);
	if(multibyteBufferCapacity > 0) 
	{
		destBuffer.Realloc(multibyteBufferCapacity);
		bytesWritten = WideCharToMultiByte(28591, 0, wideString.c_str(), -1, destBuffer.GetPtr(), destBuffer.GetCapacity(), NULL, NULL);
	}
	destBuffer.SetLength(bytesWritten);
	return (bytesWritten > 0);
}

bool FFMPEGProducer::Load(const tstring& filename) {
	filename_ = filename;
	{
		caspar::utils::DataBuffer<char> asciiFilename;
		if(ConvertWideCharToLatin1(filename, asciiFilename))
		{
			if(av_open_input_file(&pFormatContext_, asciiFilename.GetPtr(), NULL, 0, NULL) != 0)
				return false;
		}
		else
			return false;
	}

	if(av_find_stream_info(pFormatContext_) < 0)
		return false;

	if(!FindStreams())
		return false;

	//open videoStream
	if(videoStreamIndex_ != -1) {
		pVideoCodec_ = avcodec_find_decoder(pVideoCodecContext_->codec_id);
		if(pVideoCodec_ == 0)
			return false;

		if(avcodec_open(pVideoCodecContext_, pVideoCodec_ ) < 0)
			return false;

		pDecodeFrame_ = avcodec_alloc_frame();
		pRGBFrame_ = avcodec_alloc_frame();
		if(pDecodeFrame_ == NULL || pRGBFrame_ == NULL)
			return false;

		videoFrameRate_ = static_cast<double>(pVideoCodecContext_->time_base.den) / static_cast<double>(pVideoCodecContext_->time_base.num);

		if(pVideoCodec_->id == CODEC_ID_DVVIDEO) {
			bSwapFields_ = true;
		}
	}

	//open audioStream
	if(audioStreamIndex_ != -1) {
		pAudioCodec_ = avcodec_find_decoder(pAudioCodecContext_->codec_id);
		if(pAudioCodec_ == 0)
			return false;

		if(avcodec_open(pAudioCodecContext_, pAudioCodec_ ) < 0)
			return false;

		int bytesPerSec = (pAudioCodecContext_->sample_rate * pAudioCodecContext_->channels * 2);
		audioFrameSize_ = bytesPerSec / 25;
		sourceAudioFrameSize_ = static_cast<double>(bytesPerSec) / videoFrameRate_;

		//make sure the framesize is a multiple of the samplesize
		int sourceSizeMod = sourceAudioFrameSize_%(pAudioCodecContext_->channels * 2);
		if(sourceSizeMod != 0)
			sourceAudioFrameSize_ += ((pAudioCodecContext_->channels * 2)-sourceSizeMod);

		if(audioFrameSize_ == 0)
			return false;
	}

	if(pVideoCodecContext_ != 0)
	{
		pFrameBuffer_ = new unsigned char[pVideoCodecContext_->width*pVideoCodecContext_->height*4];
	}

	return true;
}

bool FFMPEGProducer::GetProducerInfo(MediaProducerInfo* pInfo) {
	if(pInfo != 0) {
		pInfo->HaveVideo = (videoStreamIndex_ != -1);
		pInfo->HaveAudio = (audioStreamIndex_ != -1);
		if(pInfo->HaveAudio) {
			pInfo->AudioChannels = pAudioCodecContext_->channels;
			pInfo->AudioSamplesPerSec = pAudioCodecContext_->sample_rate;
			pInfo->BitsPerAudioSample = 16;
		}
		return true;
	}
	return false;
}

IMediaController* FFMPEGProducer::QueryController(const tstring& id) {
	if(id == TEXT("FrameController"))
		return this;
	
	return 0;
}

bool FFMPEGProducer::FindStreams() {
	bool bReturnValue = false;

	for(int i=0; i < pFormatContext_->nb_streams; ++i) {
		if(pFormatContext_->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO && videoStreamIndex_ == -1) {
			videoStreamIndex_ = i;
			pVideoCodecContext_ = pFormatContext_->streams[videoStreamIndex_]->codec;
			bReturnValue = true;
			continue;
		}
		
		if(pFormatContext_->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO && audioStreamIndex_ == -1) {
			audioStreamIndex_ = i;
			pAudioCodecContext_ = pFormatContext_->streams[audioStreamIndex_]->codec;
			bReturnValue = true;
			continue;
		}
	}

	return bReturnValue;
}

bool FFMPEGProducer::Initialize(FrameManagerPtr pFrameManager) {
	if(pFrameManager != this->pFrameManager_) {
		if(pFrameManager == 0)
			return false;

		{
			Lock lock(*this);
			pTempFrameManager_ = pFrameManager;
		}

		initializeEvent_.Set();
		if(!worker_.IsRunning()) {
			return worker_.Start(this);
		}
	}

	return true;
}

void FFMPEGProducer::Run(HANDLE stopEvent) {
	LOG << LogLevel::Verbose << TEXT("FFMPEGProducer readAhead thread started");
	{
		Lock lock(*this);
		const caspar::FrameFormatDescription& fmtDesc = pTempFrameManager_->GetFrameFormatDescription();
	}

	if(pVideoCodecContext_ != 0)
		avpicture_fill((AVPicture*)pRGBFrame_, pFrameBuffer_, PIX_FMT_BGRA, pVideoCodecContext_->width, pVideoCodecContext_->height);

	HANDLE waitHandles[3] = { stopEvent, initializeEvent_, frameBuffer_.GetWriteWaitHandle() };

	bool bQuit = false;
	while(!bQuit) {
		HRESULT waitResult = WaitForMultipleObjects(3, waitHandles, FALSE, 1000);
		switch(waitResult)
		{
		//stop
		case (WAIT_OBJECT_0+0):
			bQuit = true;
			break;

		//change pFrameManager
		case (WAIT_OBJECT_0+1):
			{
				Lock lock(*this);
				if(pFrameManager_ == 0 && pSwsContext_ == 0 && pVideoCodecContext_ != 0) {
					double param = 0;
					pSwsContext_ = sws_getContext(pVideoCodecContext_->width, pVideoCodecContext_->height, pVideoCodecContext_->pix_fmt, pTempFrameManager_->GetFrameFormatDescription().width, pTempFrameManager_->GetFrameFormatDescription().height, PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, &param);
				}
				pFrameManager_ = pTempFrameManager_;
				pTempFrameManager_.reset();
			}

			break;

		//write possible
		case (WAIT_OBJECT_0+2):
			{
				bool bFrameHaveVideo = false;
				while(true) {
					if(pCurrentFrame_ == 0) {
						pCurrentFrame_ = pFrameManager_->CreateFrame();
					}

					if(pCurrentFrame_ != 0 && pCurrentFrame_->HasValidDataPtr()) {
						AVPacket packet;
						int readFrameResult = av_read_frame(pFormatContext_, &packet);
						if(readFrameResult >= 0) {
							if(packet.stream_index == videoStreamIndex_) {
								if(HandleVideoPacket(packet)) {
									if(audioStreamIndex_ != -1) {
										if(audioDataChunkQueue_.size() > 0) {
											pCurrentFrame_->AddAudioDataChunk(audioDataChunkQueue_.front());
											audioDataChunkQueue_.pop();

											//the frame is complete
											frameBuffer_.push_back(pCurrentFrame_);
											pCurrentFrame_.reset();
										}
										else {
											//the frame needs audio-data
											bFrameHaveVideo = true;
										}
									}
									else {
										//no audiostream availible - the frame is complete
										frameBuffer_.push_back(pCurrentFrame_);
										pCurrentFrame_.reset();
									}
								}
								else {
									//Something failed. stop playback of this clip
									bQuit = true;
								}
							}
							else if(packet.stream_index == audioStreamIndex_) {
								if(HandleAudioPacket(packet)) {
									if(bFrameHaveVideo || videoStreamIndex_ == -1) {
										pCurrentFrame_->AddAudioDataChunk(audioDataChunkQueue_.front());
										audioDataChunkQueue_.pop();
										bFrameHaveVideo = false;

										//The frame is complete
										frameBuffer_.push_back(pCurrentFrame_);
										pCurrentFrame_.reset();
									}
								}
							}
							av_free_packet(&packet);
							if(bFrameHaveVideo)
								continue;
						}
						else if(GetLoop()) {
							if(av_seek_frame(pFormatContext_, -1, 0, AVSEEK_FLAG_BACKWARD) >= 0)
								continue;
							else
								bQuit = true;
						}
						else {
							bQuit = true;
						}
					}
					else {
						pCurrentFrame_.reset();
						Sleep(0);
					}
					break;
				}
			}
			break;

		case (WAIT_TIMEOUT):
			break;

		default:
			//Iiik, Critical error
			bQuit = true;
			break;
		}
	}

	pCurrentFrame_.reset();

	FramePtr pNullFrame;
	frameBuffer_.push_back(pNullFrame);

	LOG << LogLevel::Verbose << TEXT("FFMPEGProducer readAhead thread ended");
}

bool FFMPEGProducer::OnUnhandledException(const std::exception& ex) throw() {
	try 
	{
		FramePtr pNullFrame;
		frameBuffer_.push_back(pNullFrame);

		LOG << LogLevel::Critical << TEXT("UNHANDLED EXCEPTION in FFMPEGProducer readAheadthread. Message: ") << ex.what();
	}
	catch(...) {}

	return false;
}

bool FFMPEGProducer::HandleVideoPacket(AVPacket& packet) {
	int frameFinished = 0;
	if(pVideoCodec_->id == CODEC_ID_RAWVIDEO) {
		utils::image::Shuffle(pCurrentFrame_->GetDataPtr(), packet.data, packet.size, 3, 2, 1, 0);
		return true;
	}
	else if(avcodec_decode_video(pVideoCodecContext_, pDecodeFrame_, &frameFinished, packet.data, packet.size) > 0) {
		int rowsGenerated = sws_scale(pSwsContext_, pDecodeFrame_->data, pDecodeFrame_->linesize, 0, pVideoCodecContext_->height, pRGBFrame_->data, pRGBFrame_->linesize);
		CopyFrame(pCurrentFrame_, pFrameBuffer_, pVideoCodecContext_->width, pVideoCodecContext_->height);
		//avpicture_fill((AVPicture*)pRGBFrame_, pFrameBuffer_, PIX_FMT_BGRA, pVideoCodecContext_->width, pVideoCodecContext_->height);
		return true;
	}

	return false;
}

bool FFMPEGProducer::HandleAudioPacket(AVPacket& packet) {
	bool bReturnValue = false;
	int maxChunkLength = min(audioFrameSize_, sourceAudioFrameSize_);

	int writtenBytes = FFMPEGManager::AudioDecompBufferSize-FFMPEGManager::Alignment;
	int result = avcodec_decode_audio2(pAudioCodecContext_, reinterpret_cast<int16_t*>(pAlignedAudioDecompAddr_), &writtenBytes, packet.data, packet.size);
	static int discardBytes = 0;

	if(result > 0) {
		char* pDecomp = pAlignedAudioDecompAddr_;

		while(writtenBytes > 0) {
			//if there are bytes to discard, do that first
			if(discardBytes != 0) {
				int bytesToDiscard = min(writtenBytes, discardBytes);
				pDecomp += bytesToDiscard;

				discardBytes -= bytesToDiscard;
				writtenBytes -= bytesToDiscard;
				continue;
			}

			//if we're starting on a new chunk, allocate it
			if(pCurrentAudioDataChunk_ == 0) {
				pCurrentAudioDataChunk_.reset(new caspar::audio::AudioDataChunk(audioFrameSize_));
				currentAudioDataChunkOffset_ = 0;
			}

			//either fill what's left of the chunk or copy all writtenBytes that are left
			int targetLength = min((maxChunkLength - currentAudioDataChunkOffset_), writtenBytes);
			memcpy(pCurrentAudioDataChunk_->GetDataPtr()+currentAudioDataChunkOffset_, pDecomp, targetLength);
			writtenBytes -= targetLength;

			currentAudioDataChunkOffset_ += targetLength;
			pDecomp += targetLength;

			if(currentAudioDataChunkOffset_ >= maxChunkLength) {
				if(maxChunkLength < audioFrameSize_) {
					memset(pCurrentAudioDataChunk_->GetDataPtr()+maxChunkLength, 0, audioFrameSize_-maxChunkLength);
				}
				else if(audioFrameSize_ < sourceAudioFrameSize_) {
					discardBytes = sourceAudioFrameSize_-audioFrameSize_;
				}

				audioDataChunkQueue_.push(pCurrentAudioDataChunk_);
				pCurrentAudioDataChunk_.reset();
				bReturnValue = true;
			}
		}
	}

	return bReturnValue;
}

void FFMPEGProducer::CopyFrame(FramePtr pDest, const unsigned char* pSource, int srcWidth, int srcHeight) {
	const caspar::FrameFormatDescription& destFormatDesc = pFrameManager_->GetFrameFormatDescription();

	int width = min(destFormatDesc.width, srcWidth);
	int height = min(destFormatDesc.height, srcHeight);

	if(srcWidth == destFormatDesc.width) {
		if(bSwapFields_) {
			memcpy(pDest->GetDataPtr(), pSource+destFormatDesc.width*4, width*(height-1)*4);
			memset(pDest->GetDataPtr()+(width*(height-1)*4), 0, width*4);
		}
		else {
			//copy all in one great big call
			memcpy(pDest->GetDataPtr(), pSource, width*height*4);
		}
	}
	else {
		//copy every row by itself
		for(int rowIndex=0; rowIndex < height; ++rowIndex) {
			unsigned char* pDestData = pDest->GetDataPtr() + destFormatDesc.width*4*rowIndex;
			const unsigned char* pSourceData = pSource + srcWidth*4*rowIndex;

			memcpy(pDestData, pSourceData, width*4);
		}
	}
}

}	//namespace ffmpeg
}	//namespace caspar