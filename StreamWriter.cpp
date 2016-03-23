#include "StreamWriter.h"
#include "config.h"
#include <vector>
#include <string>
#include <exception>

StreamWriter::StreamWriter(
		int viewportWidth, int viewportHeight, int num_buffers) :
	width(viewportWidth), height(viewportHeight), numPBOs(num_buffers)
{
	pbo = new GLuint[num_buffers];
	glGenBuffers(num_buffers, pbo);
	avcodec_register_all();
	AVDictionary *opts = nullptr;
	av_dict_set(&opts, "tune", "zerolatency", 0);
	av_dict_set(&opts, "preset", "ultrafast", 0);
	av_dict_set(&opts, "crf", xstr(CODEC_CRF), 0);
#ifdef RTMP_STREAM
	av_dict_set(&opts, "rtmp_live", "live", 0);
#endif // RTMP_STREAM
	AVIOContext *ioctx;
	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec)
		throw std::exception();
	avctx = avcodec_alloc_context3(codec);
	avctx->pix_fmt = AV_PIX_FMT_YUV420P;
	avctx->width = STREAM_WIDTH;
	avctx->height = STREAM_HEIGHT;
	avctx->time_base = { 1, 60 };
#ifdef UDP_STREAM
	avctx->gop_size = 0;
#endif // UDP_STREAM
	if (avcodec_open2(avctx, codec, &opts) < 0)
		throw std::exception();

	avframe = av_frame_alloc();
	avframe->format = AV_PIX_FMT_YUV420P;
	avframe->width = STREAM_WIDTH;
	avframe->height = STREAM_HEIGHT;
	avframe->linesize[0] = STREAM_WIDTH;
	avframe->linesize[1] = STREAM_WIDTH / 2;
	avframe->linesize[2] = STREAM_WIDTH / 2;
	if (av_frame_get_buffer(avframe, 0) != 0)
		throw std::exception();

	int linesize_align[AV_NUM_DATA_POINTERS];
	avcodec_align_dimensions2(avctx, &avframe->width, &avframe->height, linesize_align);
	for (int i = 0; i < numPBOs; i++)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[i]);
		glBufferData(
				GL_PIXEL_PACK_BUFFER,
				width * height * 4,
				nullptr,
				GL_STREAM_READ);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	av_register_all();
	avformat_network_init();
	avfmt = avformat_alloc_context();
	std::string filename = STREAM_PATH;
	avfmt->oformat = av_guess_format("flv", 0, 0);
	av_dict_set(&opts, "live", "1", 0);
	filename.copy(avfmt->filename, filename.size(), 0);
	avfmt->start_time_realtime = AV_NOPTS_VALUE;
	AVStream *s = avformat_new_stream(avfmt, codec);
	s->time_base = { 1, 60 };
	if (s == nullptr)
		throw std::exception();
	s->codec = avctx;
	int io_result =
		avio_open2(&ioctx, filename.c_str(), AVIO_FLAG_WRITE, nullptr, &opts);
	open = (io_result >= 0);
	if (!open)
		return;
	avfmt->pb = ioctx;
	if (avformat_write_header(avfmt, &opts) != 0)
		throw std::exception();

	swctx = sws_getContext(
			width, height, AV_PIX_FMT_BGRA,
			STREAM_WIDTH, STREAM_HEIGHT, AV_PIX_FMT_YUV420P,
			SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (swctx == nullptr)
		throw std::exception();
}

StreamWriter::~StreamWriter()
{
	if (open)
		Close();
	avformat_free_context(avfmt);
	avformat_network_deinit();
	//avcodec_free_context(&avctx);
	av_frame_free(&avframe);
	delete[] pbo;
}

void StreamWriter::WriteFrame()
{
	int buf_index = frame % numPBOs;
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[buf_index]);
	if (frame >= numPBOs)
	{
		uint8_t *data =
			(uint8_t *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		uint8_t *const srcSlice[] = { data };
		int srcStride[] = { width * 4 };
		if (data != nullptr)
			sws_scale(
					swctx,
					srcSlice, srcStride,
					0, height,
					avframe->data, avframe->linesize);
		avframe->pts += 1500;
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	}
	glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	if (frame >= numPBOs)
	{
		AVPacket *avpkt = av_packet_alloc();
		int got_packet;
		if (avcodec_encode_video2(avctx, avpkt, avframe, &got_packet) < 0)
			exit(1);
		if (got_packet == 1)
			av_interleaved_write_frame(avfmt, avpkt);
		av_packet_free(&avpkt);
	}
	frame++;
}

void StreamWriter::Close()
{
	av_write_trailer(avfmt);
	avcodec_close(avctx);
	av_free(avfmt->pb);

	open = false;
}

bool StreamWriter::IsOpen() const
{
	return open;
}
