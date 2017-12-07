#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


struct wav_header {
	uint32_t riff_id; //0x46464952
	uint32_t riff_sz;  //
	uint32_t riff_fmt; //0x45564157
	uint32_t fmt_id;  //0x20746d66
	uint32_t fmt_sz;  //16bit
	uint16_t audio_format;  //1 LPCM
	uint16_t num_channels;  //1
	uint32_t sample_rate;  //44100
	uint32_t byte_rate;  //44100*2
	uint16_t block_align;  //2
	uint16_t bits_per_sample; //16bits
	uint32_t data_id;  //0x61746164
	uint32_t data_sz;  //
};


#define COUNT 1
int main(int argc, char **argv)
{
	FILE *file = NULL;
	uint8_t *buf;
	struct wav_header header;
	int ignore_size = 0;
	int ignore_count = 0;
	int voice_count = 0;
	uint8_t start_write = 0;
	uint8_t file_temp_open = 0;
	int frames_temp = 0;
	int index = 0;
	char *file_name = (char *)malloc(20);
	FILE *file_temp;
#define THRESHOLD_AUDIO 256
#define COUNT_THRESHOLD 8
#define SECTION_AUDIO   4000 //sample datas of between -650~650 are not voice
#define VOICE_THRESHOLD 1  //at least 4*1024 bytes is voice,start capturing 
#define HIGH_THRESHOLD_FRAMES 10000
	int bytes_read = 0;
	int flag = 0;

	buf = (uint8_t *)malloc(sizeof(struct wav_header)*COUNT);

	if (buf == NULL)fprintf(stderr, "malloc fail\n");

	file = fopen(argv[1], "rb");

	if (!file)
		fprintf(stderr, "Unable to open file\n");
	else fprintf(stderr, "Open file success\n");

	fseek(file, 0, SEEK_SET);

	flag = fread(buf, sizeof(struct wav_header), COUNT, file);
	header.riff_id = *buf | *(buf + 1) << 8 | *(buf + 2) << 16 | *(buf + 3) << 24; buf += 4;
	header.riff_sz = *buf | *(buf + 1) << 8 | *(buf + 2) << 16 | *(buf + 3) << 24; buf += 4;
	header.riff_fmt = *buf | *(buf + 1) << 8 | *(buf + 2) << 16 | *(buf + 3) << 24; buf += 4;
	header.fmt_id = *buf | *(buf + 1) << 8 | *(buf + 2) << 16 | *(buf + 3) << 24; buf += 4;
	header.fmt_sz = *buf | *(buf + 1) << 8 | *(buf + 2) << 16 | *(buf + 3) << 24; buf += 4;
	header.audio_format = *buf | *(buf + 1) << 8; buf += 2;
	header.num_channels = *buf | *(buf + 1) << 8; buf += 2;
	header.sample_rate = *buf | *(buf + 1) << 8 | *(buf + 2) << 16 | *(buf + 3) << 24; buf += 4;
	header.byte_rate = *buf | *(buf + 1) << 8 | *(buf + 2) << 16 | *(buf + 3) << 24; buf += 4;
	header.block_align = *buf | *(buf + 1) << 8; buf += 2;
	header.bits_per_sample = *buf | *(buf + 1) << 8; buf += 2;
	header.data_id = *buf | *(buf + 1) << 8 | *(buf + 2) << 16 | *(buf + 3) << 24; buf += 4;
	header.data_sz = *buf | *(buf + 1) << 8 | *(buf + 2) << 16 | *(buf + 3) << 24; buf = buf + 4 - sizeof(struct wav_header);

	printf("riff_id:%X\n", header.riff_id);
	printf("riff_sz:%X\n", header.riff_sz);
	printf("riff_fmt:%X\n", header.riff_fmt);
	printf("fmt_id:%X\n", header.fmt_id);
	printf("fmt_sz:%X\n", header.fmt_sz);
	printf("audio_format:%X\n", header.audio_format);
	printf("num_channels:%X\n", header.num_channels);
	printf("sample_rate:%u\n", header.sample_rate);
	printf("byte_rate:%u\n", header.byte_rate);
	printf("block_align:%X\n", header.block_align);
	printf("bits_per_sample:%X\n", header.bits_per_sample);
	printf("data_id:%X\n", header.data_id);
	printf("data_sz:%d\n", header.data_sz);
	printf("header.size:%d\n", sizeof(struct wav_header));

	free(buf);


#define PERIOD 1024*16

	buf = (uint8_t *)malloc(PERIOD);
	int k = header.data_sz;
	int i, j = 0;
	while (k) {
		flag = fread(buf, PERIOD, 1, file);

		if (!flag)
			fprintf(stderr, "Unable to read file\n");
		if (ferror(file))
			fprintf(stderr, "File read error\n");
		if (feof(file))
			fprintf(stderr, "File reach end\n");

		//for (int i = 0; i <= PERIOD - 1; i++)printf("%X\t", *(buf + i));

		for (j = 0; j <= 16 - 1; j++) {

			for (i = 0; i <= PERIOD / 16 - 2; i += 2) {
				if ((int16_t)(*(buf + j * 1024 + i) | (*(buf + j * 1024 + i + 1)) << 8) < SECTION_AUDIO && (int16_t)(*(buf + j * 1024 + i) | *(buf + j * 1024 + i + 1) << 8) > -SECTION_AUDIO)ignore_size++;
				else ignore_size = 0;
				//printf("%d\t", (int16_t)(*(buf + i) | (*(buf + i + 1)) << 8));
			}

			//printf("%d\t", ignore_size);
			if (ignore_size <= THRESHOLD_AUDIO&&start_write == 0 && voice_count >= VOICE_THRESHOLD) {//if it's a audio period,open the file_temp
				start_write = 1;
				file_temp_open = 1;
				sprintf(file_name, "%d.wav", index);
				file_temp = fopen(file_name, "wb");
				if (!file_temp) {
					fprintf(stderr, "Unable to create temp_file '%s'\n", file_name);
					return 1;
				}
			}


			if (ignore_size > THRESHOLD_AUDIO) {
				voice_count = 0;
				ignore_count++;
			}
			else {
				voice_count++;
				ignore_count = 0;
			}

			if (ignore_count > COUNT_THRESHOLD)start_write = 0;

			if (start_write) {//,write to file_temp
				if (fwrite(buf + j * 1024, 1, PERIOD / 16, file_temp) != PERIOD / 16) {
					fprintf(stderr, "Error capturing sample\n");
					break;
				}
				bytes_read += PERIOD / 16;
			}

			else if (file_temp_open) {//
				ignore_count = 0;
				frames_temp = bytes_read / 2;
				if (frames_temp >= HIGH_THRESHOLD_FRAMES) {
					printf("Captured %d frames\n", frames_temp);
					//   write header now all information is known 
					header.data_sz = frames_temp * header.block_align;
					header.riff_sz = header.data_sz + sizeof(struct wav_header) - 8;
					fseek(file_temp, 0, SEEK_SET);
					fwrite(&header, sizeof(struct wav_header), 1, file_temp);
					printf("%d\n", index);
					index++;
					bytes_read = 0;
					fclose(file_temp);
					file_temp_open = 0;
				}
				else {
					bytes_read = 0;
					fclose(file_temp);
					file_temp_open = 0;
					continue;
				}
			}

			ignore_size = 0;
		}
		k -= PERIOD;
	}

	if (file_temp_open) {
		ignore_count = 0;
		frames_temp = bytes_read / 2;
		if (frames_temp >= HIGH_THRESHOLD_FRAMES) {
			printf("Captured %d frames\n", frames_temp);
			//   write header now all information is known 
			header.data_sz = frames_temp * header.block_align;
			header.riff_sz = header.data_sz + sizeof(struct wav_header) - 8;
			fseek(file_temp, 0, SEEK_SET);
			fwrite(&header, sizeof(struct wav_header), 1, file_temp);
			printf("%d\n", index);
			index++;
			bytes_read = 0;
			fclose(file_temp);
			file_temp_open = 0;
		}
		else {
			bytes_read = 0;
			fclose(file_temp);
			if (remove(file_name))fprintf(stderr, "Error remove error file!\n");
			file_temp_open = 0;
		}
	}

	free(file_name);
	fclose(file);

	return 0;

}