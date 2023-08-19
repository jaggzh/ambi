#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#define BUFSIZE 1024

int main(int argc, char* argv[]) {
	pa_simple *s = NULL;
	pa_sample_spec ss;
	ss.format = PA_SAMPLE_S16LE;
	ss.channels = 2;
	ss.rate = 44100;
	
	int error;
	double seconds = 3;
	int verbose = 0;
	int avg_flag = 0;
	int peak_flag = 0;
	int percent_flag = 0;
	int continuous = 0;
	int quiet = 0;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--secs") == 0) {
			if (i+1 < argc) {
				seconds = atof(argv[++i]);
			}
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
			verbose = 1;
		} else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--avg") == 0) {
			avg_flag = 1;
		} else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--peak") == 0) {
			peak_flag = 1;
		} else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cont") == 0) {
			continuous = 1;
		} else if (strcmp(argv[i], "-q") == 0) {
			quiet = 1;
		} else if (strcmp(argv[i], "-%") == 0) {
			percent_flag = 1;
		}
	}
	if (!avg_flag && !peak_flag) {
		avg_flag = peak_flag = 1;
		if (!quiet) {
			fprintf(stderr, "Defaulting to output peak and avg (-p -a).\n");
		}
	}

	if ((s = pa_simple_new(NULL, "VolumeMeter", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error)) == NULL) {
		fprintf(stderr, "pa_simple_new() failed: %s\n", pa_strerror(error));
		return 1;
	}

	setlinebuf(stdout);

	do {
		int16_t buf[BUFSIZE];
		double avg_sum = 0;
		double peak = 0;
		int total_samples = 0;

		if (!quiet) {
			fprintf(stderr, "Listening for %.3f seconds...\n", seconds);
		}

		for (int i = 0; i < seconds * ss.rate / (BUFSIZE / ss.channels); i++) {
			if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
				fprintf(stderr, "pa_simple_read() failed: %s\n", pa_strerror(error));
				goto finish;
			}
			
			for (int j = 0; j < BUFSIZE; j++) {
				avg_sum += abs(buf[j]);
				if (abs(buf[j]) > peak) {
					peak = abs(buf[j]);
				}
				total_samples++;
			}
		}

		double avg = avg_sum / total_samples;
		double max_value = 32767.0; // Max value for 16-bit audio

		if (verbose) {
			printf("Amplitude Range: [0, %d]\n", 32767);
		}

		if (avg_flag) {
			if (verbose)
				printf("Average Amplitude: ");
			if (percent_flag)
				printf("%.1f%%\n", (avg / max_value) * 100.0);
			else
				printf("%.1f\n", avg);
		}

		if (peak_flag) {
			if (verbose)
				printf("Peak Amplitude: ");
			if (percent_flag) {
				printf("%.1f%%\n", (peak / max_value) * 100.0);
			} else {
				printf("%.1f\n", peak);
			}
		}
	} while (continuous);

finish:
	if (s) {
		pa_simple_free(s);
	}

	return 0;
}
