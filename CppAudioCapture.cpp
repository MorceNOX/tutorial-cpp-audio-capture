#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <portaudio.h>
#include <portaudiocpp/Exception.hxx>

#define SAMPLE_RATE 44100.0
#define FRAMES_PER_BUFFER 512
#define NUM_CHANNELS 2

using namespace std;

static void checkErr(PaError err){
	if (err != paNoError){
		printf("PortAudio error %s\n", Pa_GetErrorText(err));
		exit(EXIT_FAILURE);
	}
}

// Full-block and half-block characters: `█`, `▀`, `▄`

static inline float max(float a, float b){
	return a > b ? a : b;
}


static inline float absolute(float a){
	return a > 0 ? a : -a;
}

static int paVMeterCallback(
		const void *inputBuffer,
		void *outputBuffer,
		unsigned long int framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		unsigned long int statusFlag,
		void *userData
		){

	float* in = (float*)inputBuffer;
	(void*)outputBuffer;

	int dispSize = 100;
	printf("\r");

	float vol_L = 0;
	float vol_R = 0;

	printf("\e[40m");

	for (unsigned long i = 0; i < framesPerBuffer * 2; i += 2){
		vol_L = max(vol_L, absolute((float)in[i]));
		vol_R = max(vol_R, absolute((float)in[i+1]));
	}

	for (int i = 0; i < dispSize; i++){
		float barProportion = i / (float)dispSize;

		// Green: volume < 1/3
		if (
				(barProportion < vol_L && barProportion < vol_R) &&
				(vol_L <= 0.33 && vol_R <= 0.33)
		){
			printf("\e[0;32m\e[40m█");

		} else if (
				barProportion < vol_L && vol_L <= 0.33
		){
			printf("\e[0;32m\e[40m▀");

		} else if (barProportion < vol_R && vol_R <= 0.33
		){
			printf("\e[0;32m\e[40m▄");

			// Yellow: volume < 2/3
		} else if (
				(barProportion <= vol_L && barProportion <= vol_R) &&
				(vol_L > 0.33 && vol_R > 0.33) &&
				(vol_L < 0.67 && vol_R < 0.67)
		){
			printf("\e[0;33m\e[40m█");

		} else if (
				barProportion <= vol_L && vol_L > 0.33 && vol_L < 0.67
		){
			printf("\e[0;33m\e[40m▀");

		} else if (
				barProportion <= vol_R && vol_R > 0.33 && vol_R < 0.67
		){
			printf("\e[0;33m\e[40m▄");

			// Red: volume >= 2/3
		} else if (
				(barProportion <= vol_L && barProportion <= vol_R) &&
				(vol_L >= 0.67 && vol_R >= 0.67)
		){
			printf("\e[0;31m\e[40m█");

		} else if (
				barProportion <= vol_L && vol_L >= 0.67
		){
			printf("\e[0;31m\e[40m▀");

		} else if (
				barProportion <= vol_R && vol_R >= 0.67
		){
			printf("\e[0;31m\e[40m▄");

			// Black bar: 0 signal
		} else {
			printf("\e[40m ");

		}
	}
	printf("\e[0m");

	fflush(stdout);

	return 0;

}

int main(int argc, char **argv) {

    PaError err;
	err = Pa_Initialize();
	checkErr(err);

	int numDevices = Pa_GetDeviceCount();
	printf("Number of devices: %d\n", numDevices);
	if (numDevices < 0){
		printf("Error getting device count.\n");
		exit(EXIT_FAILURE);
	} else if (numDevices == 0){
		printf("Error getting device count.\n");
		exit(EXIT_FAILURE);
	}

	const PaDeviceInfo* deviceInfo;

	for (int i = 0; i < numDevices; i++){
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
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(device)->defaultLowInputLatency;

	printf("\n____________________________________________________________________________________________________\n");
	printf("Input device: (%d) %s\n", device, deviceInfo->name);
	printf("Audio captured signal level (L/R):\n");
	printf("\e[0;32m1...5...10....15.......25.......33\e[0;33m.....40........50........60....66\e[0;31m........75...80.......90......100\e[0m\n");

	PaStream* stream;
	err = Pa_OpenStream(
			&stream,
			&inputParameters,
			NULL,
			Pa_GetDeviceInfo(device)->defaultSampleRate,
			FRAMES_PER_BUFFER,
			paNoFlag,
			paVMeterCallback,
			NULL
		);

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

	printf("\n");

	return EXIT_SUCCESS;

}

