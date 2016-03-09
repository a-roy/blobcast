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
		void ReceiveFrame(uint8_t *data);

	private:
		AVFormatContext *avfmt = NULL;
		AVCodecContext *avctx = NULL;
		AVFrame *avframe = NULL;
		SwsContext *swctx = NULL;
		int width;
		int height;
};
