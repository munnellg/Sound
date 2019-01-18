#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <pulse/error.h>
#include <pulse/simple.h>

#define BUFFER_SIZE 1024

#define DEFAULT_SAMPLE_FORMAT   PA_SAMPLE_S16LE
#define DEFAULT_SAMPLE_RATE     (48 * 1024)
#define DEFAULT_SAMPLE_CHANNELS 1

static struct state {
	pa_sample_spec ss;
	pa_simple *s;
	int error;

	char **args;
	int    num_args;
} state;

static void
clean_up(void)
{
	if (state.s) { pa_simple_free(state.s); }
}

static void
usage(const char *progname)
{
	fprintf(stdout, "usage: %s [options] FILE\n", progname);
	fprintf(stdout, "\n");
	fprintf(stdout, "options:\n");
	fprintf(stdout, "\t-h\n");
	fprintf(stdout, "\t\tprint this help message\n");

	clean_up();
	exit(EXIT_SUCCESS);
}

static void
die(const char *msg, ...)
{
	va_list args;
	
	va_start(args, msg);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	va_end(args);

	exit(EXIT_FAILURE);
}

static void
start_up(void)
{
	/* check to see if we should read audio from file */
	if (state.num_args > 0) {
		int fd;

		/* open the file */
		if ((fd = open(state.args[0], O_RDONLY)) < 0) {
			die("open() failed : %s", strerror(errno));
		}

		/* redirect file to stdin */
		if (dup2(fd, STDIN_FILENO) < 0) {
			die("dup2() failed : %s", strerror(errno));
		}

		close(fd);
	}

	state.s = pa_simple_new(
		NULL, 
		"sound", 
		PA_STREAM_PLAYBACK, 
		NULL, 
		"playback", 
		&state.ss, 
		NULL, 
		NULL, 
		&state.error
	);

	if (!state.s) {
		die("pa_simple_new() failed : %s", pa_strerror(state.error));
	}

}

static void
run(void)
{
	uint8_t buffer[BUFFER_SIZE];
	ssize_t r;

	for(;;) {
		/* load some audio from the file */
		if ((r = read(STDIN_FILENO, buffer, sizeof(buffer))) <= 0) {
			if (r == 0) { break; } /* EOF */

			die("read() failed : %s", strerror(errno));
		}

		/* play the audio */
		if (pa_simple_write(state.s, buffer, (size_t) r, &state.error) < 0) {
			die("pa_simple_write() failed : %s", pa_strerror(state.error));
		}
	}

	if (pa_simple_drain(state.s, &state.error) < 0) {
		die("pa_simple_drain() failed : %s", pa_strerror(state.error));
	}
}

static void
parse_args(int argc, char **argv)
{
	memset(&state, 0, sizeof(struct state));
	
	state.args = argv;
	state.ss.format = DEFAULT_SAMPLE_FORMAT;
	state.ss.rate = DEFAULT_SAMPLE_RATE;
	state.ss.channels = DEFAULT_SAMPLE_CHANNELS;

	char *progname = argv[0];

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strcmp("-h", argv[i]) == 0) {
				usage(progname);
			} else {
				die("Invalid control flag: \"%s\"", argv[i]);
			}
		} else {
			state.args[state.num_args++] = argv[i];
		}
	}
}

int
main(int argc, char *argv[])
{
	parse_args(argc, argv);

	start_up();

	run();

	clean_up();

	return EXIT_SUCCESS;
}