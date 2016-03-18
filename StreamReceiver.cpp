#include "StreamReceiver.h"
#include "config.h"
#include <exception>

StreamReceiver::StreamReceiver(
		const char *address, int viewportWidth, int viewportHeight) :
			width(viewportWidth), height(viewportHeight)
{
	AVDictionary *opts = nullptr;
	av_dict_set(&opts, "tune", "zerolatency", 0);
	av_dict_set(&opts, "preset", "ultrafast", 0);
	av_dict_set(&opts, "rtmp_live", "live", 0);
	av_dict_set(&opts, "analyzeduration", "100000", 0);
	av_register_all();
	avformat_network_init();
	if (avformat_open_input(
				&avfmt,
				address,
				nullptr,
				&opts
				) < 0)
		throw std::exception();
	if (avfmt->streams[0]->codec->codec_id == AV_CODEC_ID_NONE)
		avfmt->streams[0]->codec->codec_id = AV_CODEC_ID_H264;
	AVCodec *codec = avcodec_find_decoder(avfmt->streams[0]->codec->codec_id);
	if (codec == nullptr)
		throw std::exception();
	
	avctx = avcodec_alloc_context3(codec);
	if (avcodec_open2(avctx, nullptr, &opts) < 0)
		throw std::exception();
	
	swctx = sws_getContext(
			STREAM_WIDTH, STREAM_HEIGHT, AV_PIX_FMT_YUV420P,
			width, height, AV_PIX_FMT_BGRA,
			SWS_LANCZOS, nullptr, nullptr, nullptr);
	
	avframe = av_frame_alloc();
}

StreamReceiver::~StreamReceiver()
{
	avcodec_close(avctx);
	avcodec_free_context(&avctx);
	avformat_network_deinit();
	av_frame_free(&avframe);
}

void StreamReceiver::ReceiveFrame(uint8_t *data)
{
	AVPacket *pkt = av_packet_alloc();
	av_init_packet(pkt);
	if (av_read_frame(avfmt, pkt) < 0)
		return;
	int got_picture;
	if (avcodec_decode_video2(avctx, avframe, &got_picture, pkt) < 0)
		return;
	av_packet_unref(pkt);
	av_packet_free(&pkt);
	if (got_picture == 0)
		return;

	int linesize_align[AV_NUM_DATA_POINTERS];
	avcodec_align_dimensions2(
			avctx, &avframe->width, &avframe->height, linesize_align);

	uint8_t *const dstSlice[] = { data };
	int dstStride[] = { width * 4 };
	if (data != nullptr)
		sws_scale(
				swctx,
				avframe->data, avframe->linesize,
				0, STREAM_HEIGHT,
				dstSlice, dstStride);
	av_frame_unref(avframe);
}
