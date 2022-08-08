#!/bin/env python3
import dbus
import dbus.service
import dbus.mainloop.glib
from gi.repository import GLib
import sys
import time
import random
import string

OBJ_PATH = "/org/fubar/signal1"
INTERFACE = "org.fubar.signal1"


def _rs(l):
    return ''.join(random.choice(string.ascii_lowercase) for _ in range(l))


count = 0
prev_size = 0
diff_sum = 0.0
start = -1.0


def _spam_handler(ts, size, msg):
    global count
    global prev_size
    global diff_sum
    global start

    now = time.time()

    if size > 2:
        count += 1
        diff_sum += (now - ts)
    elif size == 1:
        # Starting a new sequence
        start = now
        diff_sum = 0.0
        count = 0
    elif size == 2:
        if prev_size > 2 and count > 10:
            # Output a result
            avg = diff_sum / float(count)
            msg_sec = float(count) / (now - start)
            mib = (prev_size * msg_sec) / float(1024 * 1024)

            print("%d,%f,%f,%f,%d" % (prev_size, avg, msg_sec, mib, count))

        diff_sum = 0.0
        count = 0
        start = now

    prev_size = size


class SignalSpammer(dbus.service.Object):

    def __init__(self, c, path):
        dbus.service.Object.__init__(self, c, path)

    @dbus.service.signal(dbus_interface=INTERFACE,
                         signature='dts')
    def Spam(self, ts, pl_len, pl):
        pass

    @dbus.service.method(dbus_interface=INTERFACE,
                         in_signature='tt', out_signature='t')
    def SpamSignal(self, num_signals, payload_size):
        sent = 0

        print("SpamSignal: %d, %d" % (num_signals, payload_size))

        pl = _rs(payload_size)

        for i in range(num_signals):
            self.Spam(time.time(), payload_size, pl)
            sent += 1

        return sent


if __name__ == '__main__':

    server = None

    if len(sys.argv) != 2:
        print("syntax: spamsignals.py [client|server]")
        sys.exit(1)

    mode = sys.argv[1]

    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()

    if mode == 'server':
        name = dbus.service.BusName(INTERFACE, bus)
        server = SignalSpammer(bus, OBJ_PATH)

    elif mode == 'client':
        s = dbus.Interface(bus.get_object(INTERFACE, OBJ_PATH), INTERFACE)
        s.connect_to_signal('Spam', _spam_handler)
    else:
        print("Invalid mode %s" % (mode))
        sys.exit(1)

    try:
        loop = GLib.MainLoop()
        loop.run()
    except KeyboardInterrupt:
        if server:
            server.remove_from_connection(bus, OBJ_PATH)
        pass