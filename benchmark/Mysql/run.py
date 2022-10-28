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
    cmd = prefix + ' /usr/local/mysql/bin/mysqld -u root --port=3308'
    print('ssh %s@%s -p %s %s' % (userName, serverAddr, serverPort, cmd))
    mysqldOutput = subprocess.run('ssh %s@%s -p %s %s' % (userName, serverAddr, serverPort, cmd),
                                  capture_output=True,
                                  shell=True, text=False)
    print('Mysql server stopped')
    # print('Stdout:', mysqldOutput.stdout)
    # print('StdErr:', mysqldOutput.stderr)


def startBenchmark():
    dockerCmd = 'docker run --rm=true --name=sbtest severalnines/sysbench sysbench --mysql-user=root --mysql-host=192.168.1.24 --mysql-port=3308 --mysql-db=sbtest --oltp-tables-count=8 --threads=16 --oltp-dist-type=uniform --oltp-read-only=on --report-interval=10 --time=20 --max-requests=0 /usr/share/sysbench/tests/include/oltp_legacy/oltp.lua run'
    sysbenchOutput = subprocess.run('ssh %s@%s -p %s %s' % ('tongping', '192.168.1.25', '22', dockerCmd),
                                    capture_output=True,
                                    shell=True, text=True)
    print('Sysbench stopped')
    print('Stdout:' + sysbenchOutput.stdout)
    print('StdErr:' + sysbenchOutput.stderr)

    # Extract throughput
    for line in sysbenchOutput.stdout.split('\n'):
        line = line.strip()
        line = line.split()
        if len(line) > 0 and line[0] == 'queries:':
            return float(line[2][1:])

    raise Exception('Cannot find throughput')


def measureMemOverhead():
    checkPidOutput = subprocess.run('ssh %s@%s -p %s ps -aux |grep mysqld' % (userName, serverAddr, serverPort),
                                    capture_output=True,
                                    shell=True, text=True)
    pidList = []
    for line in checkPidOutput.stdout.split('\n'):
        if len(line.strip()) > 0 and ('/usr/local/mysql/bin/mysqld' in line):
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


def killMysqld():
    sysbenchOutput = subprocess.run('ssh %s@%s -p %s ps -aux |grep mysqld' % (userName, serverAddr, serverPort),
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
        print('Starting mysql server......')
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

        killMysqld()

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

for prefix in prefixList:
    throughputList, memOverheadList = runIndividualTest(8, prefix)
    throughputList = rmMinMax(throughputList)
    memOverheadList = rmMinMax(memOverheadList)

    runtimeDict[prefix] = np.average(np.array(throughputList))
    memDict[prefix] = np.average(np.array(memOverheadList))

print('====> Runtime Overhead')

for prefix in prefixList:
    print('%s\t', prefix, end='')
print('')

for prefix in prefixList:
    print(str(runtimeDict[prefix]) + '\t', end='')
print('')

print('<====')

print('====> Memory Overhead')
for prefix in prefixList:
    print('%s\t', prefix, end='')
print('')

for prefix in prefixList:
    print(str(memDict[prefix]) + '\t', end='')
print('')

print('<====')
