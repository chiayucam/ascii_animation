#include <stdio.h>
#include <windows.h>
#include <MMsystem.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <getopt.h>
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
int aspectRatio = 0;
int screenWidth = 160;
int screenHeight = 60;
int fontSize = 16;
char asciiSet[] = " .:-=+*#%@";
double time_spent = 0.0;

void print_usage() {
	printf("Usage: ascii_animation.exe [-r {high|medium|low}] <video_path> \n");
	exit(2);
}

void hidecursor()
{
   HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
   CONSOLE_CURSOR_INFO info;
   info.bVisible = FALSE;
   SetConsoleCursorInfo(consoleHandle, &info);
}

void setConsoleParameters(HANDLE hStdout, int res_option, int aspect_ratio) {
	if (aspect_ratio == 0) {
		switch (res_option) {
			case 0:
				screenHeight = 45;
				screenWidth = 160;
				fontSize = 16;
				break;
			case 1:
				screenHeight = 90;
				screenWidth = 320;
				fontSize = 12;
				break;
			case 2:
				screenHeight = 135;
				screenWidth = 480;
				fontSize = 8;
				break;
		}
	} else {
		switch (res_option) {
			case 0:
				screenHeight = 60;
				screenWidth = 120;
				fontSize = 16;
				break;
			case 1:
				screenHeight = 90;
				screenWidth = 240;
				fontSize = 12;
				break;
			case 2:
				screenHeight = 135;
				screenWidth = 360;
				fontSize = 8;
				break;
		}
	}

	// Set the size of the screen buffer
	COORD coord = { screenWidth, screenHeight};
	SMALL_RECT const minimal_window = { 0, 0, 1, 1 };
	SMALL_RECT Rect;
	Rect.Top = 0;
    Rect.Left = 0;
    Rect.Bottom = screenHeight - 1;
    Rect.Right = screenWidth - 1;

	// if(!SetConsoleWindowInfo(hStdout, TRUE, &minimal_window)) {
	// 	printf("SetConsoleWindowInfo to minimal failed\n");
	// };
	if (!SetConsoleScreenBufferSize(hStdout, coord)) {
		printf("SetConsoleScreenBufferSize failed\n");
	}
	if (!SetConsoleWindowInfo(hStdout, TRUE, &Rect)) {
		printf("SetConsoleWindowInfo to Rect failed\n");
	}
	if (!SetConsoleScreenBufferSize(hStdout, coord)) {
		printf("SetConsoleScreenBufferSize failed\n");
	}
	if (!SetConsoleWindowInfo(hStdout, TRUE, &Rect)) {
		printf("SetConsoleWindowInfo to Rect failed\n");
	}

	//  change console font size
    CONSOLE_FONT_INFOEX cfi = {sizeof(cfi)};
	GetCurrentConsoleFontEx(hStdout, FALSE, &cfi);
	cfi.dwFontSize.Y = fontSize;
	SetCurrentConsoleFontEx(hStdout, FALSE, &cfi);
}

int getAspectRatio(int x, int y) {
	// support standard ratio 16:9(return 1) or 4:3(return 0)
	double value = (double)x / y;
    if (value > 1.7)
        return 1;
    else
        return 0;
}

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

static void display_frame(const AVFrame *frame, HANDLE buffer, char asciiSet[], int asciiSetLength, char *data, AVRational time_base) {
	int x, y;
    uint8_t *p0, *p1;
	DWORD  written;
	COORD pos;
	int64_t delay;
	
	char *pdata;
	
	clock_t begin = clock();
	pdata = data;
	pos.X = 0;
	pos.Y = 0;
	/* Trivial ASCII grayscale display. */
	p0 = frame->data[0];
    // puts("\033c");
	
    for (y = 0; y < frame->height; y++) {
        p1 = p0;
        for (x = 0; x < frame->width; x++) {
			*pdata = asciiSet[*(p1) / (255/asciiSetLength)];
			// *pAscii = asciiSet[*(p2) / 32];
			// putchar(" .,-+#$@"[*(p2) / 32]);
            // putchar(" .,-+#$@"[*p2 / 32]);
			// WriteConsole(hStdout, " .,-+#$@"[*(p2) / 32], 1, &cChars, NULL);
			p1++;
			pdata++;
		}
        p0 += frame->linesize[0];
    }
	WriteConsoleOutputCharacter(buffer, data, frame->width * frame->height, pos, &written);
	clock_t end = clock();
	time_spent += (double)(end - begin) / CLOCKS_PER_SEC;

	if (frame->pts != AV_NOPTS_VALUE) {
        if (last_pts != AV_NOPTS_VALUE) {
            /* sleep roughly the right amount of time;
             * usleep is in microseconds, just like AV_TIME_BASE. */
            delay = av_rescale_q(frame->pts - last_pts, time_base, AV_TIME_BASE_Q);

			// testing delay
			delay -= (int64_t)(time_spent*1000000);

            if (delay > 0 && delay < 1000000) {
				usleep(delay);
			}
        }
        last_pts = frame->pts;
    }
}


int main(int argc, char **argv) {
	// check input argument
	if (argc < 2 || argc > 4) {
		print_usage();
		return -1;
	}

	int option;
	int res_option = 1;
	while ((option = getopt(argc, argv, "r:")) != -1) {
		switch (option) {
			case 'r':
				if (strcmp(optarg, "high") == 0) {
					res_option = 2;
				}
				if (strcmp(optarg, "medium") == 0) {
					res_option = 1;
				}
				if (strcmp(optarg, "low") == 0) {
					res_option = 0;
				}
				break;
		}
	}
	

	ShowWindow(GetConsoleWindow() , SW_MAXIMIZE);

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	hStdin = GetStdHandle(STD_INPUT_HANDLE);

	hidecursor();

	// allocating memory for header information using AVFormatContext structure
	AVFormatContext *pFormatContext = avformat_alloc_context();

	// open file and read header
	if (avformat_open_input(&pFormatContext, argv[argc-1], NULL, NULL) !=0) {
		av_log(0, AV_LOG_FATAL, "Wasn't possible opening the file: %s", argv[argc-1]);
		return -1;
	}
	system("cls");
	printf("opening file %s\n", argv[argc-1]);

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
	

	printf("Press enter to continue...\n");
	getchar();
	system("cls");

	// // play music
	// PlaySound(TEXT("./video/bad_apple.mp4"), NULL, SND_ASYNC);
	
	setConsoleParameters(hStdout, res_option, getAspectRatio(pCodecParameters->width, pCodecParameters->height));

	// decode video
	AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
	avcodec_parameters_to_context(pCodecContext, pCodecParameters);
	avcodec_open2(pCodecContext, pCodec, NULL);

	AVPacket *pPacket = av_packet_alloc();
	AVFrame *pFrame = av_frame_alloc();

	struct SwsContext *pScaledContext;
	pScaledContext = sws_getContext(pCodecParameters->width, pCodecParameters->height, AV_PIX_FMT_YUV420P, screenWidth, screenHeight, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

	AVFrame *pScaledFrame = av_frame_alloc();
	int scaledFrameSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, screenWidth, screenHeight, 1);
	uint8_t *pScaledFrameBuffer = (uint8_t *)av_malloc(scaledFrameSize*sizeof(uint8_t));
	av_image_fill_arrays(pScaledFrame->data, pScaledFrame->linesize, pScaledFrameBuffer, AV_PIX_FMT_YUV420P, screenWidth, screenHeight, 1);

	char *data;
	data = malloc(sizeof(char)*screenWidth*screenHeight);

	clock_t begin, end;
	
	while (av_read_frame(pFormatContext, pPacket) >=0) {
		if (pPacket->stream_index != video_stream_index) {
			continue;
		}
		avcodec_send_packet(pCodecContext, pPacket);
		avcodec_receive_frame(pCodecContext, pFrame);

		time_spent = 0.0;
		begin = clock();
		sws_scale(pScaledContext, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pFrame->height, pScaledFrame->data, pScaledFrame->linesize);
		// pScaledFrame->pts = pScaledFrame->best_effort_timestamp;
		pScaledFrame->pts = pFrame->best_effort_timestamp;
		pScaledFrame->pkt_dts = pFrame->pkt_dts;
		pScaledFrame->width = screenWidth;
		pScaledFrame->height = screenHeight;
		pScaledFrame->coded_picture_number = pFrame->coded_picture_number;
		pScaledFrame->display_picture_number = pFrame->display_picture_number;
		end = clock();
		time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		
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

		
		
		display_frame(pScaledFrame, hStdout, asciiSet, sizeof(asciiSet)/sizeof(asciiSet[0])-1, data, pFormatContext->streams[video_stream_index]->time_base);
	}

	// PlaySound(NULL, NULL, SND_SYNC);
	

	avformat_close_input(&pFormatContext);
	avformat_free_context(pFormatContext);
	av_frame_free(&pFrame);
	av_frame_free(&pScaledFrame);
	av_packet_free(&pPacket);
	avcodec_free_context(&pCodecContext);
	av_free(pScaledFrameBuffer);
	free(data);
	return 0;
}
