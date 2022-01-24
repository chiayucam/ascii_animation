#include <stdio.h>
#include <windows.h>
#include <MMsystem.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/log.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
// #pragma comment(lib, "winmm.lib")

static int64_t last_pts = AV_NOPTS_VALUE;
HANDLE hStdout;
HANDLE hStdin;
HANDLE hdBuffer;
int screenWidth = 160;
int screenHeight = 60;

void usleep(int64_t usec) 
{ 
    HANDLE timer; 
    LARGE_INTEGER ft;

    ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL); 
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
    WaitForSingleObject(timer, INFINITE); 
    CloseHandle(timer); 
}

static void write_buffer(const AVFrame *frame, HANDLE buffer, char asciiSet[]) {
	int x, y;
    uint8_t *p0, *p1;
	unsigned long cChars;
	char ascii[1];

	/* Trivial ASCII grayscale display. */
	p0 = frame->data[0];
    // puts("\033c");
    for (y = 0; y < frame->height; y++) {
        p1 = p0;
        for (x = 0; x < frame->width; x++) {
			// ascii[0] = asciiSet[*(p2) / 32];
			// WriteConsoleOutputCharacter()
			// putchar(" .,-+#$@"[*(p2) / 32]);
            // putchar(" .,-+#$@"[*p2 / 32]);
			// WriteConsole(hStdout, " .,-+#$@"[*(p2) / 32], 1, &cChars, NULL);
			// p2++;
			// p3++;
		}
        // putchar('\n');
        p0 += frame->linesize[0];
		p1 += frame->linesize[0];
    }
}

static void display_frame(const AVFrame *frame,  const AVFrame *prevFrame, AVRational time_base, HANDLE hStdout , HANDLE hdBuffer)
{
    int x, y;
    uint8_t *p0, *p1, *p2, *p3;
    int64_t delay;
	COORD pos;
	unsigned long cChars;
	char asciiSet[] = " .,-+#$@";
	char ascii[1];

    if (frame->pts != AV_NOPTS_VALUE) {
        if (last_pts != AV_NOPTS_VALUE) {
            /* sleep roughly the right amount of time;
             * usleep is in microseconds, just like AV_TIME_BASE. */
            delay = av_rescale_q(frame->pts - last_pts, time_base, AV_TIME_BASE_Q);
            if (delay > 0 && delay < 1000000) {
				usleep(delay);
			}
        }
        last_pts = frame->pts;
    }

    /* Trivial ASCII grayscale display. */
    p0 = frame->data[0];
	p1 = prevFrame->data[0];
    // puts("\033c");
    for (y = 0; y < frame->height; y++) {
        p2 = p0;
		p3 = p1;
        for (x = 0; x < frame->width; x++) {
			if (*p2 != *p3) {
				pos.X = x;
				pos.Y = y;
				SetConsoleCursorPosition(hStdout, pos);
				ascii[0] = asciiSet[*(p2) / 32];
				WriteConsole(hStdout, ascii, 1, &cChars, NULL);
				// putchar(" .,-+#$@"[*(p2) / 32]);
			}
            // putchar(" .,-+#$@"[*p2 / 32]);
			// WriteConsole(hStdout, " .,-+#$@"[*(p2) / 32], 1, &cChars, NULL);
			p2++;
			p3++;
		}
        // putchar('\n');
        p0 += frame->linesize[0];
		p1 += frame->linesize[0];
    }
    // fflush(stdout);
	// printf("\nframe: %d", frame->coded_picture_number);
}


// decode packets into frames
int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecCollntext, AVFrame *pFrame);

int main(int argc, const char *argv[]) {
	// check input argument
	if (argc < 2) {
		printf("require media file as input\n");
		return -1;
	}

	ShowWindow(GetConsoleWindow() , SW_MAXIMIZE);

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	hStdin = GetStdHandle(STD_INPUT_HANDLE);

	hdBuffer = CreateConsoleScreenBuffer(GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,CONSOLE_TEXTMODE_BUFFER,NULL);

	// SMALL_RECT m_rectWindow = { 0, 0, 1, 1 };
	// SetConsoleWindowInfo(hStdout, TRUE, &m_rectWindow);

	// Set the size of the screen buffer
	COORD coord = { (short)screenWidth, (short)screenHeight };
	SetConsoleScreenBufferSize(hStdout, coord);
	SetConsoleScreenBufferSize(hdBuffer, coord);


	// //  change console font size
	// HANDLE hcsb = CreateFileA("CONOUT$", GENERIC_WRITE | GENERIC_READ,
    //     FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 

    // CONSOLE_FONT_INFOEX cfi = {sizeof(cfi)};
	// GetCurrentConsoleFontEx(hcsb, FALSE, &cfi);
	// cfi.dwFontSize.X = 18;
	// cfi.dwFontSize.Y = 16;
	// SetCurrentConsoleFontEx(hcsb, FALSE, &cfi);
	
	// allocating memory for header information using AVFormatContext structure
	AVFormatContext *pFormatContext = avformat_alloc_context();

	// open file and read header
	if (avformat_open_input(&pFormatContext, argv[1], NULL, NULL) !=0) {
		av_log(0, AV_LOG_FATAL, "Wasn't possible opening the file: %s", argv[1]);
		return -1;
	}
	system("cls");
	printf("opening file %s\n", argv[1]);

	// get stream info
	avformat_find_stream_info(pFormatContext, NULL);

	const AVCodec *pCodec = NULL;
	AVCodecParameters *pCodecParameters = NULL;
	int video_stream_index = -1;

	for (int i = 0; i < pFormatContext->nb_streams; i++) {
		AVCodecParameters *pLocalCodecParameters = NULL;
		pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

		const AVCodec *pLocalCodec = NULL;
		pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

		if (pLocalCodec==NULL) {
			printf("unsupported codec\n");
			continue;
		}

		printf("===========================================================\n");
		printf("codec %s ID %d bit_rate %I64d\n", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
		printf("-----------------------------------------------------------\n");
		printf("AVStream->time_base before open coded %d/%d\n", pFormatContext->streams[i]->time_base.num, pFormatContext->streams[i]->time_base.den);
		printf("AVStream->r_frame_rate before open coded %d/%d\n", pFormatContext->streams[i]->r_frame_rate.num, pFormatContext->streams[i]->r_frame_rate.den);
		printf("AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
		printf("\n");
		printf("AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);
		printf("\n");

		if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (video_stream_index == -1) {
				video_stream_index = i;
				pCodec = pLocalCodec;
				pCodecParameters = pLocalCodecParameters;
			}
			printf("video codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
		}
		else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
			printf("audio codec: %d channels, sample rate %d\n", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
		}
	}
	
	// play music
	// PlaySound(TEXT("./video/bad_apple.wav"), NULL, SND_ASYNC);

	printf("Press enter to continue...\n");
	getchar();
	system("cls");

	// decode video
	AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
	avcodec_parameters_to_context(pCodecContext, pCodecParameters);
	avcodec_open2(pCodecContext, pCodec, NULL);

	AVPacket *pPacket = av_packet_alloc();
	AVFrame *pFrame = av_frame_alloc();

	struct SwsContext *pScaledContext;
	pScaledContext = sws_getContext(pCodecParameters->width, pCodecParameters->height, AV_PIX_FMT_YUV420P, 160, 60, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

	AVFrame *pScaledFrame = av_frame_alloc();
	int scaledFrameSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, 160, 60, 1);
	uint8_t *pScaledFrameBuffer = (uint8_t *)av_malloc(scaledFrameSize*sizeof(uint8_t));
	av_image_fill_arrays(pScaledFrame->data, pScaledFrame->linesize, pScaledFrameBuffer, AV_PIX_FMT_YUV420P, 160, 60, 1);

	AVFrame *pPrevScaledFrame = av_frame_alloc();
	uint8_t *pPrevScaledFrameBuffer = (uint8_t *)av_malloc(scaledFrameSize*sizeof(uint8_t));
	av_image_fill_arrays(pPrevScaledFrame->data, pPrevScaledFrame->linesize, pPrevScaledFrameBuffer, AV_PIX_FMT_YUV420P, 160, 60, 1);

	// unsigned char *data;
	// data = (unsigned char *)malloc(pFrame->height * pFrame->width);
	// unsigned char data[480*360];
	
	while (av_read_frame(pFormatContext, pPacket) >=0) {
		if (pPacket->stream_index != video_stream_index) {
			continue;
		}
		avcodec_send_packet(pCodecContext, pPacket);
		avcodec_receive_frame(pCodecContext, pFrame);

		sws_scale(pScaledContext, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pFrame->height, pScaledFrame->data, pScaledFrame->linesize);
		// pScaledFrame->pts = pScaledFrame->best_effort_timestamp;
		pScaledFrame->pts = pFrame->pts;
		pScaledFrame->pkt_dts = pFrame->pkt_dts;
		pScaledFrame->width = 160;
		pScaledFrame->height = 60;
		pScaledFrame->coded_picture_number = pFrame->coded_picture_number;
		pScaledFrame->display_picture_number = pFrame->display_picture_number;

		
		// printf(
		// "Frame %c (%d) pts %" PRId64 " dts %" PRId64 " key_frame %d width: %d height: %d [coded_picture_number %d, display_picture_number %d] %u\n",
		// av_get_picture_type_char(pScaledFrame->pict_type),
		// pCodecContext->frame_number,
		// pScaledFrame->pts,
		// pScaledFrame->pkt_dts,
		// pScaledFrame->key_frame,
		// pScaledFrame->width,
		// pScaledFrame->height,
		// pScaledFrame->coded_picture_number,
		// pScaledFrame->display_picture_number,
		// (unsigned int)pScaledFrame->data[0][0]
		// );

		// printf(
		// "Frame %c (%d) pts %" PRId64 " dts %" PRId64 " key_frame %d width: %d height: %d [coded_picture_number %d, display_picture_number %d] %u\n",
		// av_get_picture_type_char(pFrame->pict_type),
		// pCodecContext->frame_number,
		// pFrame->pts,
		// pFrame->pkt_dts,
		// pFrame->key_frame,
		// pFrame->width,
		// pFrame->height,
		// pFrame->coded_picture_number,
		// pFrame->display_picture_number,
		// (unsigned int)pFrame->data[0][0]
		// );

		
		
		if (pCodecContext->frame_number==1) {
			av_frame_copy(pPrevScaledFrame, pScaledFrame);
			continue;
		}
		display_frame(pScaledFrame, pPrevScaledFrame, pFormatContext->streams[video_stream_index]->time_base, hStdout, hdBuffer);
		av_frame_copy(pPrevScaledFrame, pScaledFrame);
	}


	// PlaySound(NULL, NULL, SND_SYNC);

	avformat_close_input(&pFormatContext);
	avformat_free_context(pFormatContext);
	av_frame_free(&pFrame);
	av_frame_free(&pScaledFrame);
	av_frame_free(&pPrevScaledFrame);
	av_packet_free(&pPacket);
	avcodec_free_context(&pCodecContext);

	return 0;
}
