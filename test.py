import dbus
from dbus.mainloop.glib import DBusGMainLoop
import time
import math

bus = dbus.SystemBus(mainloop=DBusGMainLoop())
signals = dbus.Interface(bus.get_object("org.fubar.signal1",
                                                "/org/fubar/signal1"),
                                                "org.fubar.signal1")

for i in range(5, 18):
    size = math.pow(2, i)
    start = time.time()
    signals.SpamSignal(10, 1)
    signals.SpamSignal(10000, size)
    signals.SpamSignal(10, 2)
    end = time.time()
    print('i= %d, time= %f' % (size, float(end - start)))

