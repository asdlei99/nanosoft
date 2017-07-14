
#include <ffcairo/avc_feedagent.h>

#include <sys/time.h>

/**
* Конструктор
*/
AVCFeedStream::AVCFeedStream(AVCFeedAgent *a): agent(a)
{
}

/**
* Деструктор
*/
AVCFeedStream::~AVCFeedStream()
{
}

/**
* Обработчик фрейма
*/
void AVCFeedStream::handleFrame()
{
	//printf("AVCFeedStream::handleFrame()\n");
	agent->handleFrame(avFrame);
}

/**
* Конструктор
*/
AVCFeedAgent::AVCFeedAgent(NetDaemon *d): AVCAgent(d)
{
}

/**
* Деструктор
*/
AVCFeedAgent::~AVCFeedAgent()
{
}

/**
* Обработчик записи пакета в поток
*/
int AVCFeedAgent::write_packet(void *opaque, uint8_t *buf, int buf_size)
{
	AVCFeedAgent *agent = (AVCFeedAgent*)opaque;
	if ( agent->sendData((const char *)buf, buf_size) ) return buf_size;
	else AVERROR(EAGAIN);
}

/**
* Обработчик установления соединения
* 
* Этот обработчик вызывается после успешного вызова connect().
* В ассинхронном режиме это обычно происходит до полноценного
* установления соединения, но движок уже готов буферизовать данные
* и как только соединение будет завершено, данные будут отправлены.
* Если соединение установить не удасться, то будет вызван обработчик
* onError()
*/
void AVCFeedAgent::onConnect()
{
	printf("AVCFeedAgent::onConnect()\n");
	
	avc_packet_t pkt;
	pkt.channel = 0;
	pkt.type = 1;
	avc_set_packet_len(&pkt, sizeof(pkt));
	if ( ! sendPacket(&pkt) )
	{
		printf("fail to sendPacket()\n");
		exit(-1);
	}
	
	if ( ! muxer->openAVIO() )
	{
		printf("fail to openAVIO()\n");
		exit(-1);
	}
	
	//int ret = av_write_frame(muxer->avFormat, 0);
	//printf("av_write_frame(flush), ret = %d\n", ret);
}

/**
* Обработчик фрейма
*/
void AVCFeedAgent::handleFrame(AVFrame *avFrame)
{
	iFrame++;
	/*
	scale_pic->scale(pic->avFrame, avFrame);
	char sFrameId[80];
	snprintf(sFrameId, sizeof(sFrameId), "out/frame%04d.png", iFrame);
	printf("avFrame.pts: %"PRId64", key_frame=%d\n", avFrame->pts, avFrame->key_frame);
	pic->savePNG(sFrameId);
	*/
	
	scale_pic->scale(vout->avFrame, avFrame);
	int64_t pts = vout->avFrame->pts;
	if ( pts == AV_NOPTS_VALUE ) pts = 0;
	printf("avFrame.pts: %"PRId64", key_frame=%d\n", pts, vout->avFrame->key_frame);
	
	if ( avFrame->pts != AV_NOPTS_VALUE )
	{
		vout->avFrame->pts = av_rescale_q(avFrame->pts, vin->avStream->time_base, vout->avStream->time_base);
	}
	else
	{
		vout->avFrame->pts = AV_NOPTS_VALUE;
	}
	
	sendFrame();
}

/**
* Отправить фрейм в выходной поток
*/
int AVCFeedAgent::sendFrame()
{
	AVPacket pkt = { 0 };
	pkt.data = NULL;
	pkt.size = 0;
	av_init_packet(&pkt);
	
	int got_packet = 0;
	
	if ( ! vout->encode() )
	{
		printf("frame[%d] encode failed\n", iFrame);
		failStreaming();
		return AVERROR_UNKNOWN;
	}
	
	while ( 1 )
	{
		if ( ! vout->recv_packet(&pkt, got_packet) )
		{
			printf("recv_packet() failed\n");
			failStreaming();
			return AVERROR_UNKNOWN;
		}
		//printf("recv_packet ok, got_packet = %s\n", (got_packet ? "yes" : "no"));
		
		if ( got_packet )
		{
			int ret = av_interleaved_write_frame(muxer->avFormat, &pkt);
			if ( ret < 0 )
			{
				printf("av_interleaved_write_frame() failed\n");
				failStreaming();
			}
			
			av_packet_unref(&pkt);
			got_packet = 0;
		}
		else
		{
			break;
		}
	}
	return 0;
}

/**
* Отправить данные в поток
*/
bool AVCFeedAgent::sendData(const char *buf, int size)
{
	avc_payload_t pkt;
	//printf("AVCFeedAgent::sendData(%d) { full_pkt %lu, rem %lu }\n", size, (size/sizeof(pkt.buf)), (size%sizeof(pkt.buf)));
	
	while ( size > 0 )
	{
		int sz = size;
		if ( sz > sizeof(pkt.buf) ) sz = sizeof(pkt.buf);
		memcpy(pkt.buf, buf, sz);
		buf += sz;
		size -= sz;
		avc_set_packet_len(&pkt, sz + 4);
		pkt.channel = 2;
		pkt.type = AVC_PAYLOAD;
		//printf("payload bytes: %d\n", avc_packet_payload(&pkt));
		bool ret = sendPacket(&pkt);
		if ( ! ret )
		{
			// данные должны отправиться либо целиком, либо не отправляться
			// вообще, так что если произошла ошибка с каким-либо пакетом,
			// то прерываем сеанс
			printf("AVCFeedAgent::sendData(), sendPacket() failed\n");
			failStreaming();
			return false;
		}
	}
	
	return true;
}

/**
* Завершить стриминг
*
* Временная функция для прерывания стрима в случае непредвиденных ошибок
*/
void AVCFeedAgent::failStreaming()
{
	// TODO
	printf("AVCFeedAgent::failStreaming()\n");
	exit(-1);
}

/**
* Открыть веб-камеру
*/
int AVCFeedAgent::openWebcam(const char *fname)
{
	demux = new FFCDemuxer();
	if ( ! demux->open(fname) )
	{
		printf("failed open video device\n");
		return AVERROR_UNKNOWN;
	}
	
	return 0;
}

/**
* Открыть видео поток
*/
bool AVCFeedAgent::openVideoStream()
{
	// найти видео-поток
	int video_stream = demux->findVideoStream();
	if ( video_stream < 0 )
	{
		printf("fail to find video stream\n");
		return false;
	}
	printf("video stream #%d\n", video_stream);
	
	// присоединить обработчик потока
	vin = new AVCFeedStream(this);
	demux->bindStream(video_stream, vin);
	
	// открыть декодер видео
	if ( ! vin->openDecoder() )
	{
		printf("stream[%d] openDecoder() failed\n", video_stream);
		return false;
	}
	
	int width = vin->avStream->codecpar->width;
	int height = vin->avStream->codecpar->height;
	printf("video size: %dx%d\n", width, height);
	
	opts.width = width;
	opts.height = height;
	opts.pix_fmt = AV_PIX_FMT_YUV420P;
	opts.bit_rate = 2000000;
	opts.time_base = (AVRational){ 1, 25 };
	opts.gop_size = 5;
	
	return true;
}

/**
* Открыть исходящий стрим
*/
int AVCFeedAgent::openFeed(int width, int height, int64_t bit_rate)
{
	opts.width = width;
	opts.height = height;
	opts.bit_rate = bit_rate;
	
	muxer = new FFCMuxer();
	
	int ret = muxer->createContext("avi");
	if ( ret < 0 )
	{
		printf("failed to createContext(avi)\n");
		return ret;
	}
	
	uint8_t *avio_ctx_buffer = NULL;
	size_t avio_ctx_buffer_size = 4096;
	
	avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
	if (!avio_ctx_buffer)
	{
		printf("av_malloc failed\n");
		return AVERROR_UNKNOWN;
	}

	avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 1, this/*user pointer*/, NULL, &write_packet, NULL);
	if ( !avio_ctx )
	{
		printf("avio_alloc_context() failed\n");
		return AVERROR_UNKNOWN;
	}
	
	muxer->avFormat->pb = avio_ctx;
	
	// add video stream
	AVCodecID video_codec = muxer->defaultVideoCodec();
	if ( video_codec == AV_CODEC_ID_NONE)
	{
		printf("error: video_codec = NONE\n");
		return AVERROR_UNKNOWN;
	}
	
	opts.codec_id = video_codec;
	
	vout = muxer->createVideoStream(&opts);
	
	if ( ! vout->openEncoder(&opts) )
	{
		printf("openEncoder() failed\n");
		return AVERROR_UNKNOWN;
	}
	
	av_dump_format(muxer->avFormat, 0, 0, 1);
	
	iFrame = 0;
	
	scale = new FFCScale();
	scale->init_scale(vout->avFrame, vin->avFrame);
	
	// отладочный код
	pic = FFCImage::createImage(width, height);
	if ( pic.getObject() == NULL )
	{
		printf("fail to create FFCImage\n");
		return AVERROR_UNKNOWN;
	}
	
	scale_pic = new FFCScale();
	scale_pic->init_scale(pic->avFrame, vin->avFrame);
	
	return 0;
}

/**
* Таймер
*/
void AVCFeedAgent::onTimer()
{
	struct timeval tv;
	static int t0 = 0;
	
	//printf("onTimer()\n");
	for(int i = 0; i < 1; i++ )
	{
		gettimeofday(&tv, 0);
		int t1 = (tv.tv_sec % 100) * 1000 + (tv.tv_usec / 1000);
		
		bool ret = demux->readFrame();
		
		gettimeofday(&tv, 0);
		int t2 = (tv.tv_sec % 100) * 1000 + (tv.tv_usec / 1000);
		int ds = (t2 - t1) / 1000;
		int dms = (t2 - t1) % 1000;
		printf("readFrame() time: %d.%03d\n", ds, dms);
		
		ds = (t2 - t0) / 1000;
		dms = (t2 - t0) % 1000;
		printf("readFrame() time2: %d.%03d\n", ds, dms);
		t0 = t2;
		
		if ( ret )
		{
			//printf("frame readed\n");
		}
		else
		{
			printf("readFrame failed\n");
			break;
		}
	}
	//printf("onTimer() leave\n");
}
