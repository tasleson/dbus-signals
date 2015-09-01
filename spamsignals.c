#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <systemd/sd-bus.h>
#include <sys/time.h>
#include <errno.h>

sd_bus *bus = NULL;

#define OBJ "/org/fubar/signal1"
#define INT "org.fubar.signal1"

#define FILTER "type='signal',sender='"INT"'"

static void _rs(char *dest, size_t l) {
    char set[] = "abcdefghijklmnopqrstuvwxyz";
    while (l-- > 0) {
        size_t i = (double) rand() / RAND_MAX * (sizeof set - 1);
        *dest++ = set[i];
    }
    *dest = '\0';
}

static double time_time()
{
	struct timeval t;
	if (gettimeofday(&t, (struct timezone *)NULL) == 0)
            return (double)t.tv_sec + (t.tv_usec * 0.000001);
	return 0.0;
}

static uint64_t count = 0;
static uint64_t prev_size = 0;
static double diff_sum = 0.0;
static double start = -1.0;

static int bus_signal_cb(sd_bus_message *m, void *user_data, sd_bus_error
							*ret_error)
{
	int r = 0;
	double ts = 0;
	double now;
	const char *msg = NULL;
	uint64_t size = 0;

	now = time_time();

	r = sd_bus_message_read(m, "dts", &ts, &size, &msg);
	if (r < 0) {
		fprintf(stderr, "Failed to parse signal message: %s\n", strerror(-r));
		return -1;
    }

	if (size > 2) {
		count += 1;
		diff_sum += (now - ts);
	} else if (size == 1) {
		start = now;
		diff_sum = 0.0;
		count = 0;
	} else if (size == 2) {
		if (prev_size > 2 && count > 10) {
			double avg = diff_sum / (double)count;
			double msg_sec = (double)count / (now - start);
			double mib = (prev_size * msg_sec) / (double)(1024 * 1024);
			printf("%d,%f,%f,%f, %d\n",prev_size, avg, msg_sec, mib, count);
		}
		diff_sum = 0.0;
        count = 0;
        start = now;
	}

	prev_size = size;
	sd_bus_message_unref(m);
	return 0;
}

static int generate_signals(sd_bus_message *m, void *userdata, sd_bus_error
							*ret_error) {
	uint64_t num_signals, payload_size;
	uint64_t i = 0;
	double c_time;
	int r;
	struct timeval ts;
	char *pl = NULL;

	/* Read the parameters */
	r = sd_bus_message_read(m, "tt", &num_signals, &payload_size);
	if (r < 0) {
		fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
		return r;
	}

	printf("SpamSignal: %"PRIu64", %"PRIu64 "\n", num_signals,
			payload_size);

	pl = (char*)malloc(payload_size+1);
	_rs(pl, payload_size);

	for ( i = 0; i < num_signals; i++ ) {
		c_time = time_time();
		r = sd_bus_emit_signal(bus, OBJ, INT,
				"Spam", "dts", c_time, payload_size , pl);
		if (r < 0) {
			fprintf(stderr, "Failed to emit signal: %s\n", strerror(-r));
			free(pl);
			return sd_bus_reply_method_return(m, "t", i);
		}
	}
	free(pl);
	return sd_bus_reply_method_return(m, "t", num_signals);
}

static const sd_bus_vtable signal_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("SpamSignal", "tt", "t", generate_signals, 0),
        SD_BUS_VTABLE_END
};

int main(int argc, char *argv[]) {
        sd_bus_slot *slot = NULL;
        int r;
		char *mode = NULL;

		if (argc != 2) {
			fprintf(stderr, "syntax: %s [server|client]\n", argv[0]);
			return 1;
		}

		mode = argv[1];

		/* Connect to system bus */
        r = sd_bus_open_system(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s\n",
						strerror(-r));
                goto finish;
        }

		if (!strcmp("server", mode)) {
			r = sd_bus_add_object_vtable(bus,
										 &slot,
										 OBJ,	/* object path */
										 INT,   /* interface name */
										 signal_vtable,
										 NULL);
			if (r < 0) {
					fprintf(stderr, "Failed to issue method call: %s\n",
							strerror(-r));
					goto finish;
			}

			/* Take a well-known service name so that clients can find us */
			r = sd_bus_request_name(bus, INT, 0);
			if (r < 0) {
					fprintf(stderr, "Failed to acquire service name: %s\n",
							strerror(-r));
					goto finish;
			}

		} else if(!strcmp("client", mode)) {
			r = sd_bus_add_match(bus, &slot, FILTER, bus_signal_cb, NULL);
			if (r < 0) {
				fprintf(stderr, "Failed: sd_bus_add_match: %s\n", strerror(-r));
				goto finish;
			}
		} else {
			fprintf(stderr, "Invalid operating mode %s", mode);
			return 1;
		}

		for (;;) {
			/* Process requests */
			r = sd_bus_process(bus, NULL);
			if (r < 0) {
				fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
				goto finish;
			}
			if (r > 0) {
				continue;
			}

			r = sd_bus_wait(bus, (uint64_t) -1);
			if (r < 0) {
				fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
				goto finish;
			}
		}

finish:
        sd_bus_slot_unref(slot);
        sd_bus_unref(bus);
        return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}