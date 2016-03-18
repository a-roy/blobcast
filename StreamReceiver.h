extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class StreamReceiver
{
	public:
		StreamReceiver(
				const char *address, int viewportWidth, int viewportHeight);
		~StreamReceiver();
		StreamReceiver(const StreamReceiver&) = delete;
		StreamReceiver& operator=(const StreamReceiver&) = delete;
		StreamReceiver(StreamReceiver&& other) = default;
		StreamReceiver& operator=(StreamReceiver&& other) = default;
		void ReceiveFrame(uint8_t *data);

	private:
		AVFormatContext *avfmt = nullptr;
		AVCodecContext *avctx = nullptr;
		AVFrame *avframe = nullptr;
		SwsContext *swctx = nullptr;
		int width;
		int height;
};
