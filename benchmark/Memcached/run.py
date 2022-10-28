import os
import subprocess
import threading
import time
import numpy as np
import os

userName = 'root'
serverAddr = '192.168.1.24'
serverPort = '6022'


def startServer(prefix):
    print('Starting MysqlServer...')
    cmd = prefix + ' /usr/local/bin/memcached -u root --port=3308'
    print('ssh %s@%s -p %s %s' % (userName, serverAddr, serverPort, cmd))
    mysqldOutput = subprocess.run('ssh %s@%s -p %s %s' % (userName, serverAddr, serverPort, cmd),
                                  capture_output=True,
                                  shell=True, text=False)
    print('Memcached server stopped')
    # print('Stdout:', mysqldOutput.stdout)
    # print('StdErr:', mysqldOutput.stderr)


def startBenchmark():
    dockerCmd = 'docker run --rm redislabs/memtier_benchmark:latest --server=192.168.1.24 --port=3308 -P memcache_binary '
    sysbenchOutput = subprocess.run('ssh %s@%s -p %s %s' % ('root', '192.168.1.24', '6022', dockerCmd),
                                    capture_output=True,
                                    shell=True, text=True)
    print('memtier stopped')
    print('Stdout:' + sysbenchOutput.stdout)
    print('StdErr:' + sysbenchOutput.stderr)

    # Extract throughput
    for line in sysbenchOutput.stdout.split('\n'):
        line = line.strip()
        if len(line) > 0 and line.startswith('Totals'):
            line = line.split()
            return float(line[1])

    raise Exception('Cannot find throughput')


def measureMemOverhead():
    checkPidOutput = subprocess.run('ssh %s@%s -p %s ps -aux |grep memcached' % (userName, serverAddr, serverPort),
                                    capture_output=True,
                                    shell=True, text=True)
    pidList = []
    for line in checkPidOutput.stdout.split('\n'):
        if len(line.strip()) > 0 and ('/usr/local/bin/memcached' in line) and ('grep' not in line):
            pid = line.split()[1]
            pidList.append(pid)

    totalMemOverheadList = []
    for pid in pidList:
        memOverheadOutput = subprocess.run(
            'ssh %s@%s -p %s cat /proc/%s/status |grep VmPeak' % (userName, serverAddr, serverPort, pid),
            capture_output=True,
            shell=True, text=True)
        totalMemOverheadList.append(int(memOverheadOutput.stdout.split()[1]))
    return np.sum(np.array(totalMemOverheadList))


def killMemcached():
    sysbenchOutput = subprocess.run('ssh %s@%s -p %s ps -aux |grep memcached' % (userName, serverAddr, serverPort),
                                    capture_output=True,
                                    shell=True, text=True)
    print(sysbenchOutput.stdout.split('\n'))
    for mysqldLine in sysbenchOutput.stdout.split('\n'):
        if len(mysqldLine.strip()) == 0:
            continue
        print(mysqldLine)
        mysqldPid = mysqldLine.split()[1]

        killOutput = subprocess.run('ssh %s@%s -p %s kill -9 %s' % (userName, serverAddr, serverPort, mysqldPid),
                                    capture_output=True,
                                    shell=True, text=True)
        print(killOutput.stdout)
    pass


def runIndividualTest(repeatTimes, prefix):
    throughputList = []
    memOverheadList = []

    for i in range(repeatTimes):
        print('Starting memcached server......')
        t1 = threading.Thread(target=startServer, args=[prefix])
        t1.start()
        time.sleep(5)

        print('Starting benchmark client......')
        throughput = round(startBenchmark())
        memOverhead = round(measureMemOverhead())
        print("====> %dth result: \t %f prefix=%s " % (i, throughput, memOverhead), prefix)
        throughputList.append(throughput)
        memOverheadList.append(memOverhead)
        # Kill mysqld

        killMemcached()

        t1.join()
    return throughputList, memOverheadList


def rmMinMax(src: list):
    if len(src) >= 3:
        src.remove(min(src))
        src.remove(max(src))
    return src


runtimeDict = {}
memDict = {}

prefixList = ['', '/usr/lib/linux-tools/5.4.0-109-generic/perf record --call-graph dwarf',
              'LD_PRELOAD=/media/umass/datasystem/steven/Scaler/cmake-build-release/libHook-c/libScalerHook-HookAutoAsm-C.so']
prefixList = ['', '/usr/lib/linux-tools/5.4.0-109-generic/perf record --call-graph dwarf',
              'LD_PRELOAD=/media/umass/datasystem/steven/Scaler/cmake-build-release/libHook-c/libScalerHook-HookAutoAsm-C.so']
prefixList = ['', '/opt/intel/oneapi/vtune/latest/bin64/vtune --collect hotspots -knob sampling-mode=sw']
prefixList = ['', '/usr/lib/linux-tools/5.4.0-109-generic/perf record --call-graph dwarf',
              'LD_PRELOAD=/media/umass/datasystem/steven/Scaler/cmake-build-release/libHook-c/libScalerHook-HookAutoAsm-C.so',
              '/opt/intel/oneapi/vtune/latest/bin64/vtune --collect hotspots -knob sampling-mode=sw']

for prefix in prefixList:
    throughputList, memOverheadList = runIndividualTest(8, prefix)
    throughputList = rmMinMax(throughputList)
    memOverheadList = rmMinMax(memOverheadList)

    runtimeDict[prefix] = np.average(np.array(throughputList))
    memDict[prefix] = np.average(np.array(memOverheadList))

print('====> Runtime Overhead')

for prefix in prefixList:
    print('%s\t' % prefix, end='')
print('')

for prefix in prefixList:
    print(str(runtimeDict[prefix]) + '\t', end='')
print('')

print('<====')

print('====> Memory Overhead')
for prefix in prefixList:
    print('%s\t' % prefix, end='')
print('')

for prefix in prefixList:
    print(str(memDict[prefix]) + '\t', end='')
print('')

print('<====')
