# dbus-signals
Basic performance testing for dbus signals

#### Trying it out
1. Copy the configuration file to allow service to be run on the system bus
  * `# cp org.fubar.signal1.conf /etc/dbus-1/system.d/`
2. Build the C version
  * `# ./build.sh`
3. Run the test service
  * `# ./spamsignals server`
4. Run the test client
  * `# ./spamsignals client`
5. Run the testcase
  * `# python test.py`
6. Look at stdout form test client
```
32,0.000032,53504.235126,1.632820, 10000
64,0.000031,53895.914153,3.289546, 10000
128,0.000032,54784.320333,6.687539, 10000
256,0.000032,54166.180231,13.224165, 10000
512,0.000033,52019.149200,25.399975, 10000
1024,0.000033,53904.849543,52.641455, 10000
2048,0.000141,52412.028151,102.367242, 10000
4096,0.007057,33988.447544,132.767373, 6243
8192,0.012978,19174.779152,149.802962, 3563
16384,0.016047,10207.838931,159.497483, 1936
32768,0.015827,5245.088101,163.909003, 1158
65536,0.015932,2635.173577,164.698349, 756
131072,0.015743,1333.503221,166.687903, 984
```
Columns are:
payload byte size, average diff time, messages/sec, MiB/sec, Number of signals received

#### Notes:
* To run python server/client verson change ./spamsignals to ./spamsignals.py
* To enable kdbus, append kdbus=1 to kernel command line while having the kdbus kernel module located somewhere where is can be found, like: ```/lib/modules/`uname -r`/kernel/ipc/kdbus/```

