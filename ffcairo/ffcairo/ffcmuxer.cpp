
#include <ffcairo/ffcmuxer.h>

#include <stdio.h>

/**
* Конструктор
*/
FFCOutputStream::FFCOutputStream(AVStream *st): avStream(st), avCodecCtx(NULL)
{
	if ( st ) avCodecCtx = st->codec;
}

/**
* Деструктор
*/
FFCOutputStream::~FFCOutputStream()
{
}

/**
* Convert valid timing fields (timestamps / durations) in a packet from
* one timebase to another. 
*/
void FFCOutputStream::rescale_ts(AVPacket *pkt)
{
	av_packet_rescale_ts(pkt, avCodecCtx->time_base, avStream->time_base);
	pkt->stream_index = avStream->index;
}

/**
* Конструктор
*/
FFCVideoOutput::FFCVideoOutput(AVStream *st): FFCOutputStream(st), scaleCtx(NULL)
{
}

/**
* Деструктор
*/
FFCVideoOutput::~FFCVideoOutput()
{
}

/**
* Открыть кодек
*/
bool FFCVideoOutput::openEncoder(const FFCVideoOptions *opts)
{
	avCodecCtx = avStream->codec;
	avCodecCtx->codec_id  = opts->codec_id;
	avCodecCtx->width     = opts->width;
	avCodecCtx->height    = opts->height;
	avCodecCtx->pix_fmt   = opts->pix_fmt;
	avCodecCtx->bit_rate  = opts->bit_rate;
	avStream->time_base   = opts->time_base;
	avCodecCtx->time_base = avStream->time_base;
	avCodecCtx->gop_size  = opts->gop_size;
	
	if ( avCodecCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO )
	{
		/* just for testing, we also add B frames */
		avCodecCtx->max_b_frames = 2;
	}
	
	if ( avCodecCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO )
	{
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		 * This does not happen with normal video, it just happens here as
		 * the motion of the chroma plane does not match the luma plane. */
		avCodecCtx->mb_decision = 2;
	}
	
	int ret = avcodec_open2(avCodecCtx, NULL, NULL);
	if ( ret < 0 )
	{
		printf("avcodec_open2() failed\n");
		return false;
	}
	
	avFrame = av_frame_alloc();
	if ( !avFrame )
	{
		printf("av_frame_alloc() failed\n");
		return false;
	}
	
	avFrame->width  = opts->width;
	avFrame->height = opts->height;
	avFrame->format = opts->pix_fmt;
	
	/* allocate the buffers for the frame data */
	// TODO разобраться в форматом пикселей, почему два типа и как их конвертировать!!!
	ret = avpicture_alloc((AVPicture *)avFrame, opts->pix_fmt, opts->width, opts->height);
	if ( ret < 0 )
	{
		printf("avpicture_alloc() failed\n");
		return false;
	}
	
	return true;
}

/**
* Инициализация маштабирования
*/
bool FFCVideoOutput::initScale(int srcWidth, int srcHeight, AVPixelFormat srcFmt)
{
	scaleCtx = sws_getContext(srcWidth, srcHeight, srcFmt,
		avFrame->width, avFrame->height, avCodecCtx->pix_fmt, SWS_BILINEAR,
		NULL, NULL, NULL);
	
	return scaleCtx != 0;
}

/**
* Инициализация маштабирования
*/
bool FFCVideoOutput::initScale(ptr<FFCImage> pic)
{
	return initScale(pic->width, pic->height, PIX_FMT_BGRA);
}

/**
* Маштабировать картику
*/
void FFCVideoOutput::scale(AVFrame *pFrame)
{
	sws_scale(scaleCtx, pFrame->data, pFrame->linesize,
		0, avFrame->height, avFrame->data, avFrame->linesize);
}

/**
* Маштабировать картинку
*/
void FFCVideoOutput::scale(ptr<FFCImage> pic)
{
	sws_scale(scaleCtx, pic->avFrame->data, pic->avFrame->linesize,
		0, avFrame->height, avFrame->data, avFrame->linesize);
}

/**
* Кодировать кадр
*/
bool FFCVideoOutput::encode(AVPacket *avpkt, int *got_packet_ptr)
{
	int ret = avcodec_encode_video2(avCodecCtx, avpkt, avFrame, got_packet_ptr);
	if ( ret < 0 )
	{
		printf("avcodec_encode_video2() failed\n");
		return false;
	}
	
	return true;
}

/**
* Кодировать кадр с маштабированием
*/
bool FFCVideoOutput::encode(ptr<FFCImage> pic, AVPacket *avpkt, int *got_packet_ptr)
{
	scale(pic);
	return encode(avpkt, got_packet_ptr);
}

/**
* Конструктор
*/
FFCMuxer::FFCMuxer()
{
}

/**
* Деструктор
*/
FFCMuxer::~FFCMuxer()
{
}

/**
* Создать файл
*
* На самом деле эта функция не создает файла, а только подготавливает
* контекст, который надо будет еще донастроить, в частности создать
* потоки и настроить кодеки, после чего вызвать функцию openFile()
* которая начнет запись файла.
*/
bool FFCMuxer::createFile(const char *fname)
{
	avformat_alloc_output_context2(&avFormat, NULL, NULL, fname);
	if ( !avFormat )
	{
		printf("avformat_alloc_output_context2() failed\n");
		return false;
	}
	
	return true;
}

/**
* Вернуть кодек по умолчанию для аудио
*/
AVCodecID FFCMuxer::defaultAudioCodec()
{
	return avFormat->oformat->audio_codec;
}

/**
* Вернуть кодек по умолчанию для видео
*/
AVCodecID FFCMuxer::defaultVideoCodec()
{
	return avFormat->oformat->video_codec;
}

/**
* Вернуть кодек по умолчанию для субтитров
*/
AVCodecID FFCMuxer::defaultSubtitleCodec()
{
	return avFormat->oformat->subtitle_codec;
}

/**
* Создать поток
*/
AVStream* FFCMuxer::createStream(AVCodecID codec_id)
{
	AVCodec *codec = avcodec_find_encoder(codec_id);
	if ( !codec )
	{
		printf("encoder not found\n");
		return NULL;
	}
	
	AVStream *avStream = avformat_new_stream(avFormat, codec);
	if ( ! avStream )
	{
		printf("avformat_new_stream() failed\n");
		// TODO нужно ли как-то освободжать AVCodec* и как это делать?
		return NULL;
	}
	
	printf("stream[%d] created\n", avStream->index);
	return avStream;
}

/**
* Создать видео-поток
*/
FFCVideoOutput* FFCMuxer::createVideoStream(const FFCVideoOptions *opts)
{
	AVStream *avStream = createStream(opts->codec_id);
	if ( !avStream )
	{
		printf("fail to create video stream\n");
		return NULL;
	}
	
	FFCVideoOutput *vo = new FFCVideoOutput(avStream);
	
	return vo;
}

/**
* Открыть файл
*
* Открывает файл и записывает заголовки, перед вызовом должен
* быть создан контекст, потоки, настроены кодеки.
*/
bool FFCMuxer::openFile(const char *fname)
{
	int ret;
	
	/* open the output file, if needed */
	if (!(avFormat->oformat->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&avFormat->pb, fname, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			fprintf(stderr, "Could not open '%s'\n", fname);
			return false;
		}
	}
	
	ret = avformat_write_header(avFormat, NULL);
	if ( ret < 0 )
	{
		fprintf(stderr, "avformat_write_header() failed\n");
		return false;
	}
	
	return true;
}

/**
* Записать пакет
*/
bool FFCMuxer::writeFrame(AVPacket *pkt)
{
	int ret = av_interleaved_write_frame(avFormat, pkt);
	return ret == 0;
}

/**
* Закрыть файл
*
* Записывает финальные данные и закрывает файл.
*/
bool FFCMuxer::closeFile()
{
	/* Write the trailer, if any. The trailer must be written before you
	 * close the CodecContexts open when you wrote the header; otherwise
	 * av_write_trailer() may try to use memory that was freed on
	 * av_codec_close(). */
	av_write_trailer(avFormat);
	
	avformat_free_context(avFormat);
}