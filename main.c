#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#define BUFSIZE 1024

int continuous = 0;

void usage() {
	printf("Usage: ambi [options]\n"
	       "\n"
	       "Listen to default audio input and calculate avg and/or peak amplitudes.\n"
	       "\n"
	       "Options:\n"
	       "  -s #.# (--secs)  Seconds of audio to process at a time\n"
	       "  -v (--verbose)   Increase verbosity \n"
	       "  -c (--cont)      Continuous mode (keeps listening and outputting)\n"
	       "                   (If you request -r/--rc and -c together, sending\n"
	       "                   SIGUSR1 (kill -USR1 {pid}) will cause us to exit\n"
	       "                   with the return value. See -r below.)\n"
	       " Two types of processing for now (both may be specified):\n"
	       "  -a (--avg)       Output average amplitude\n"
	       "  -p (--peak)      Output peak amplitude (default)\n"
	       " Number format choices:\n"
	       "  -%% (--perc)     Output percentages (##.##%%) instead of raw integer values\n"
	       /* "  -2 (--8)         Output as range 0-255 (8 bit)\n" */
	       " Can return the reading as an error code too:\n"
	       "  -r (--rc)        Return code mode: Returns the last evaluated value as\n"
	       "                   the programs return/error code.\n"
	       "                   This returns 0-255 even if -%% is selected for visual,\n"
	       "                   Also, it outputs either avg or peak (-a or -p). If\n"
	       "                   you request both it will only output peak.\n"
	       "  -h (--help)      Me!\n"
	);
}

void handle_sigusr1(int sig) { // User requested we exit with value as rc
    continuous=0;
}

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
	/* int bit8_flag = 0; */ // This was intended for rc value of 0-100 instead of -255
	int returncode_flag = 0;
	int rc_type_peak = 1;
	int rc=0;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--secs") == 0) {
			if (i+1 < argc) {
				seconds = atof(argv[++i]);
			}
		} else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
			verbose = 1;
		} else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--avg")) {
			avg_flag = 1;
			rc_type_peak = 0;
		} else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--peak")) {
			peak_flag = 1;
			rc_type_peak = 1;
		} else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--cont")) {
			continuous = 1;
		} else if (!strcmp(argv[i], "-%")) {
			percent_flag = 1;
		} else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--rc")) {
			returncode_flag = 1;
		/* } else if (!strcmp(argv[i], "-2") || !strcmp(argv[i], "-8")) { */
		/* 	bit8_flag = 1; */
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage();
			exit(0);
		}
	}
	if (returncode_flag) {
		signal(SIGUSR1, handle_sigusr1);
	}

	if (avg_flag && peak_flag) {
		rc_type_peak = 1;
	} else if (!avg_flag && !peak_flag) {
		peak_flag = 1;
		rc_type_peak = 1;
		if (verbose) {
			fprintf(stderr, "Defaulting to peak output.\n");
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

		if (verbose) {
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

		if (avg_flag && peak_flag) {
			if (verbose)
				printf("avg peak: ");
			if (percent_flag)
				printf("%.2f%% %.2f%%\n",
					(avg / max_value) * 100.0,
					(peak / max_value) * 100.0);
			else
				printf("%d %d\n",
					(int)avg,
					(int)peak);
		} else {
			if (avg_flag) {
				if (verbose)
					printf("Average Amplitude: ");
				if (percent_flag)
					printf("%.2f%%\n", (avg / max_value) * 100.0);
				else
					printf("%d\n", (int)avg);
			}

			if (peak_flag) {
				if (verbose)
					printf("Peak Amplitude: ");
				if (percent_flag) {
					printf("%.2f%%\n", (peak / max_value) * 100.0);
				} else {
					printf("%d\n", (int)peak);
				}
			}
		}

		if (returncode_flag) { // doing redundant math for now
			if (rc_type_peak)
				rc=(peak / max_value) * 255;
			else
				rc=(avg / max_value) * 255;
		}
	} while (continuous);

finish:
	if (s) {
		pa_simple_free(s);
	}

	if (returncode_flag) {
		exit(rc);
	}
	return 0;
}
