#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>

#include <portaudio.h>
#include <portaudiocpp/Exception.hxx>
#include <fftw3.h>

#define SAMPLE_RATE 44100.0
#define FRAMES_PER_BUFFER 512
#define NUM_CHANNELS 2

#define SPECTRO_FREQ_START 20
#define SPECTRO_FREQ_END 20000

using namespace std;

typedef struct {
	double* in;
	double* out;
	fftw_plan p;
	int startIndex;
	int spectroSize;
} streamCallbackData;

static streamCallbackData* spectroData;



static void checkErr(PaError err) {
	if (err != paNoError) {
		printf("PortAudio error %s\n", Pa_GetErrorText(err));
		exit(EXIT_FAILURE);
	}
}

// Full-block and half-block characters: `█`, `▀`, `▄`

// Block characters: `▁`, `▂`, `▃`, `▄`, `▅`, `▆`, `▇`, `█`

static inline float max(float a, float b) { // @suppress("Unused static function")
	return a > b ? a : b;
}
static inline float minor(float a, float b) {
	return a < b ? a : b;
}
static inline float absolute(float a) { // @suppress("Unused static function")
	return a > 0 ? a : -a;
}

static int streamCallback(
		const void *inputBuffer,
		void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo *timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData) {

	float *in = (float*) inputBuffer;
	(void) outputBuffer;

	streamCallbackData* callbackData = (streamCallbackData*)userData;

	int dispSize = 100;
	printf("\r");

	for(unsigned long i = 0; i < framesPerBuffer; i++){
		callbackData->in[i] = in[i * NUM_CHANNELS];
	}

	fftw_execute(callbackData->p);

	for (int i = 0; i < dispSize; i++){
		double proportion = pow(i / (double)dispSize, 4);
		double freq = callbackData->out[
						(int)ceil(callbackData->startIndex
						+ proportion * callbackData->spectroSize)];
		if(freq < 0.00000001){
			printf(" ");
		} else if(freq < 0.125){
			printf("▁");
		} else if(freq < 0.25){
			printf("▂");
		} else if(freq < 0.375){
			printf("▃");
		} else if(freq < 0.5){
			printf("▄");
		} else if(freq < 0.625){
			printf("▅");
		} else if(freq < 0.75){
			printf("▆");
		} else if(freq < 0.875){
			printf("▇");
		} else if (freq >= 0.875){
			printf("█");
		}

	}

	fflush(stdout);

	return 0;

}

int main(int argc, char **argv) {

	PaError err;
	err = Pa_Initialize();
	checkErr(err);

	spectroData = (streamCallbackData*)malloc(sizeof(streamCallbackData));
	spectroData->in = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
    spectroData->out = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
    if(spectroData->in == NULL || spectroData->out == NULL){
    	printf("Could not allocate spectro data.\n");
    	exit(EXIT_FAILURE);
    }

    spectroData->p = fftw_plan_r2r_1d(
    		FRAMES_PER_BUFFER,
			spectroData->in,
			spectroData->out,
			FFTW_R2HC,
			FFTW_ESTIMATE
    );
    double sampleRatio = FRAMES_PER_BUFFER / SAMPLE_RATE;

    spectroData->startIndex = ceil(sampleRatio * SPECTRO_FREQ_START);
    spectroData->spectroSize = minor(ceil(sampleRatio * SPECTRO_FREQ_END),
			FRAMES_PER_BUFFER / 2.0
    ) - spectroData->startIndex;


    int numDevices = Pa_GetDeviceCount();
	printf("Number of devices: %d\n", numDevices);
	if (numDevices < 0) {
		printf("Error getting device count.\n");
		exit(EXIT_FAILURE);
	} else if (numDevices == 0) {
		printf("Error getting device count.\n");
		exit(EXIT_FAILURE);
	}

	const PaDeviceInfo *deviceInfo;

	for (int i = 0; i < numDevices; i++) {
		deviceInfo = Pa_GetDeviceInfo(i);
		printf("Device %d\n", i);
		printf("    name: %s\n", deviceInfo->name);
		printf("    maxInputChannels: %d\n", deviceInfo->maxInputChannels);
		printf("    maxOutputChannels: %d\n", deviceInfo->maxOutputChannels);
		printf("    defaultSampleRate: %f\n", deviceInfo->defaultSampleRate);
	}

	int device = Pa_GetDefaultInputDevice();

	PaStreamParameters inputParameters;

	memset(&inputParameters, 0, sizeof(inputParameters));
	inputParameters.channelCount = NUM_CHANNELS; //Pa_GetDeviceInfo(device)->maxInputChannels;
	inputParameters.device = device;
	inputParameters.hostApiSpecificStreamInfo = NULL;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency =
	Pa_GetDeviceInfo(device)->defaultLowInputLatency;

	printf("\n____________________________________________________________________________________________________\n");
	printf("Input device: (%d) %s\n", device, deviceInfo->name);
	printf("Audio captured frequency logarithmic spectrogram:\n");

	printf("\n20Hz               39   99 156  312 417 625 880 1.25 1.6  2.5 3.3  4k 5k  6.6k    10k            20kHz");
	printf("\n|▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁|▁▁▁▁|▁▁▁|▁▁▁▁|▁▁|▁▁▁|▁▁▁|▁▁▁|▁▁▁|▁▁▁▁▁|▁▁▁|▁▁▁|▁▁|▁▁▁▁|▁▁▁▁▁▁▁|▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁|\n");
	printf("\n");


	PaStream *stream;
	err = Pa_OpenStream(
			&stream,
			&inputParameters,
			NULL,
			Pa_GetDeviceInfo(device)->defaultSampleRate,
			FRAMES_PER_BUFFER,
			paNoFlag,
			streamCallback,
			spectroData);

	checkErr(err);
	err = Pa_StartStream(stream);
	checkErr(err);

	// Execute until <ENTER> is pressed
	cin.get();

	// Execute for 10 seconds
	//Pa_Sleep(10 * 1000);

	err = Pa_StopStream(stream);
	checkErr(err);

	err = Pa_CloseStream(stream);
	checkErr(err);

	err = Pa_Terminate();
	checkErr(err);

	fftw_destroy_plan(spectroData->p);
	fftw_free(spectroData->in);
	fftw_free(spectroData->out);
	fftw_free(spectroData);

	printf("\n");

	return EXIT_SUCCESS;

}

