from copy import copy

redisOutput = '''
====== PING_INLINE ======                                                   
  100000 requests completed in 1.89 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 6)
50.000% <= 0.359 milliseconds (cumulative count 51533)
75.000% <= 0.391 milliseconds (cumulative count 82477)
87.500% <= 0.399 milliseconds (cumulative count 89634)
93.750% <= 0.415 milliseconds (cumulative count 95493)
96.875% <= 0.431 milliseconds (cumulative count 96887)
98.438% <= 0.479 milliseconds (cumulative count 98508)
99.219% <= 0.583 milliseconds (cumulative count 99252)
99.609% <= 0.639 milliseconds (cumulative count 99656)
99.805% <= 0.663 milliseconds (cumulative count 99830)
99.902% <= 0.679 milliseconds (cumulative count 99911)
99.951% <= 0.695 milliseconds (cumulative count 99962)
99.976% <= 0.719 milliseconds (cumulative count 99976)
99.988% <= 1.063 milliseconds (cumulative count 99988)
99.994% <= 1.439 milliseconds (cumulative count 99994)
99.997% <= 1.463 milliseconds (cumulative count 99997)
99.998% <= 1.479 milliseconds (cumulative count 99999)
99.999% <= 1.495 milliseconds (cumulative count 100000)
100.000% <= 1.495 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
21.312% <= 0.207 milliseconds (cumulative count 21312)
37.467% <= 0.303 milliseconds (cumulative count 37467)
93.699% <= 0.407 milliseconds (cumulative count 93699)
98.668% <= 0.503 milliseconds (cumulative count 98668)
99.420% <= 0.607 milliseconds (cumulative count 99420)
99.969% <= 0.703 milliseconds (cumulative count 99969)
99.981% <= 0.807 milliseconds (cumulative count 99981)
99.990% <= 1.103 milliseconds (cumulative count 99990)
99.991% <= 1.407 milliseconds (cumulative count 99991)
100.000% <= 1.503 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 52798.31 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.318     0.112     0.359     0.415     0.551     1.495
====== PING_MBULK ======                                                   
  100000 requests completed in 1.95 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.127 milliseconds (cumulative count 189)
50.000% <= 0.367 milliseconds (cumulative count 50882)
75.000% <= 0.391 milliseconds (cumulative count 84413)
87.500% <= 0.399 milliseconds (cumulative count 91970)
93.750% <= 0.407 milliseconds (cumulative count 95350)
96.875% <= 0.423 milliseconds (cumulative count 97387)
98.438% <= 0.455 milliseconds (cumulative count 98535)
99.219% <= 0.511 milliseconds (cumulative count 99230)
99.609% <= 0.575 milliseconds (cumulative count 99617)
99.805% <= 0.615 milliseconds (cumulative count 99831)
99.902% <= 0.639 milliseconds (cumulative count 99936)
99.951% <= 0.647 milliseconds (cumulative count 99959)
99.976% <= 0.663 milliseconds (cumulative count 99984)
99.988% <= 0.671 milliseconds (cumulative count 99992)
99.994% <= 0.679 milliseconds (cumulative count 99996)
99.997% <= 0.695 milliseconds (cumulative count 99997)
99.998% <= 0.735 milliseconds (cumulative count 99999)
99.999% <= 0.743 milliseconds (cumulative count 100000)
100.000% <= 0.743 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
18.756% <= 0.207 milliseconds (cumulative count 18756)
30.742% <= 0.303 milliseconds (cumulative count 30742)
95.350% <= 0.407 milliseconds (cumulative count 95350)
99.181% <= 0.503 milliseconds (cumulative count 99181)
99.793% <= 0.607 milliseconds (cumulative count 99793)
99.997% <= 0.703 milliseconds (cumulative count 99997)
100.000% <= 0.807 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 51203.28 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.325     0.120     0.367     0.407     0.487     0.743
====== SET ======                                                   
  100000 requests completed in 2.02 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 1)
50.000% <= 0.375 milliseconds (cumulative count 56262)
75.000% <= 0.391 milliseconds (cumulative count 86113)
87.500% <= 0.399 milliseconds (cumulative count 92475)
93.750% <= 0.407 milliseconds (cumulative count 95008)
96.875% <= 0.423 milliseconds (cumulative count 96907)
98.438% <= 0.471 milliseconds (cumulative count 98566)
99.219% <= 0.575 milliseconds (cumulative count 99245)
99.609% <= 0.615 milliseconds (cumulative count 99626)
99.805% <= 0.647 milliseconds (cumulative count 99835)
99.902% <= 0.663 milliseconds (cumulative count 99919)
99.951% <= 0.679 milliseconds (cumulative count 99964)
99.976% <= 0.687 milliseconds (cumulative count 99984)
99.988% <= 0.695 milliseconds (cumulative count 99991)
99.994% <= 0.711 milliseconds (cumulative count 99995)
99.997% <= 0.751 milliseconds (cumulative count 99997)
99.998% <= 0.791 milliseconds (cumulative count 99999)
99.999% <= 0.823 milliseconds (cumulative count 100000)
100.000% <= 0.823 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
16.116% <= 0.207 milliseconds (cumulative count 16116)
21.632% <= 0.303 milliseconds (cumulative count 21632)
95.008% <= 0.407 milliseconds (cumulative count 95008)
98.814% <= 0.503 milliseconds (cumulative count 98814)
99.548% <= 0.607 milliseconds (cumulative count 99548)
99.993% <= 0.703 milliseconds (cumulative count 99993)
99.999% <= 0.807 milliseconds (cumulative count 99999)
100.000% <= 0.903 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 49504.95 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.336     0.112     0.375     0.407     0.543     0.823
====== GET ======                                                   
  100000 requests completed in 1.98 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 13)
50.000% <= 0.375 milliseconds (cumulative count 53777)
75.000% <= 0.391 milliseconds (cumulative count 78233)
87.500% <= 0.407 milliseconds (cumulative count 90642)
93.750% <= 0.423 milliseconds (cumulative count 94424)
96.875% <= 0.447 milliseconds (cumulative count 97333)
98.438% <= 0.479 milliseconds (cumulative count 98605)
99.219% <= 0.591 milliseconds (cumulative count 99247)
99.609% <= 0.639 milliseconds (cumulative count 99639)
99.805% <= 0.663 milliseconds (cumulative count 99810)
99.902% <= 0.687 milliseconds (cumulative count 99930)
99.951% <= 0.695 milliseconds (cumulative count 99955)
99.976% <= 0.711 milliseconds (cumulative count 99988)
99.994% <= 0.719 milliseconds (cumulative count 99996)
99.997% <= 0.727 milliseconds (cumulative count 100000)
100.000% <= 0.727 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
18.552% <= 0.207 milliseconds (cumulative count 18552)
28.073% <= 0.303 milliseconds (cumulative count 28073)
90.642% <= 0.407 milliseconds (cumulative count 90642)
98.819% <= 0.503 milliseconds (cumulative count 98819)
99.355% <= 0.607 milliseconds (cumulative count 99355)
99.974% <= 0.703 milliseconds (cumulative count 99974)
100.000% <= 0.807 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 50454.09 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.332     0.112     0.375     0.431     0.551     0.727
====== INCR ======                                                   
  100000 requests completed in 1.98 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.127 milliseconds (cumulative count 275)
50.000% <= 0.375 milliseconds (cumulative count 60935)
75.000% <= 0.383 milliseconds (cumulative count 77986)
87.500% <= 0.391 milliseconds (cumulative count 89438)
93.750% <= 0.399 milliseconds (cumulative count 94561)
96.875% <= 0.415 milliseconds (cumulative count 97276)
98.438% <= 0.439 milliseconds (cumulative count 98583)
99.219% <= 0.471 milliseconds (cumulative count 99276)
99.609% <= 0.559 milliseconds (cumulative count 99624)
99.805% <= 0.615 milliseconds (cumulative count 99824)
99.902% <= 0.647 milliseconds (cumulative count 99906)
99.951% <= 0.679 milliseconds (cumulative count 99960)
99.976% <= 0.719 milliseconds (cumulative count 99980)
99.988% <= 0.759 milliseconds (cumulative count 99988)
99.994% <= 0.775 milliseconds (cumulative count 99996)
99.997% <= 0.783 milliseconds (cumulative count 99998)
99.998% <= 0.799 milliseconds (cumulative count 99999)
99.999% <= 0.823 milliseconds (cumulative count 100000)
100.000% <= 0.823 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
18.156% <= 0.207 milliseconds (cumulative count 18156)
23.268% <= 0.303 milliseconds (cumulative count 23268)
96.441% <= 0.407 milliseconds (cumulative count 96441)
99.451% <= 0.503 milliseconds (cumulative count 99451)
99.803% <= 0.607 milliseconds (cumulative count 99803)
99.972% <= 0.703 milliseconds (cumulative count 99972)
99.999% <= 0.807 milliseconds (cumulative count 99999)
100.000% <= 0.903 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 50530.57 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.329     0.120     0.375     0.407     0.463     0.823
====== LPUSH ======                                                   
  100000 requests completed in 1.99 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 1)
50.000% <= 0.375 milliseconds (cumulative count 56065)
75.000% <= 0.391 milliseconds (cumulative count 87206)
87.500% <= 0.399 milliseconds (cumulative count 94347)
96.875% <= 0.407 milliseconds (cumulative count 97239)
98.438% <= 0.423 milliseconds (cumulative count 98731)
99.219% <= 0.447 milliseconds (cumulative count 99322)
99.609% <= 0.479 milliseconds (cumulative count 99653)
99.805% <= 0.575 milliseconds (cumulative count 99817)
99.902% <= 0.615 milliseconds (cumulative count 99909)
99.951% <= 0.647 milliseconds (cumulative count 99954)
99.976% <= 0.671 milliseconds (cumulative count 99979)
99.988% <= 0.687 milliseconds (cumulative count 99991)
99.994% <= 0.695 milliseconds (cumulative count 99995)
99.997% <= 0.703 milliseconds (cumulative count 99997)
99.998% <= 0.719 milliseconds (cumulative count 99999)
99.999% <= 0.727 milliseconds (cumulative count 100000)
100.000% <= 0.727 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
18.550% <= 0.207 milliseconds (cumulative count 18550)
22.141% <= 0.303 milliseconds (cumulative count 22141)
97.239% <= 0.407 milliseconds (cumulative count 97239)
99.719% <= 0.503 milliseconds (cumulative count 99719)
99.889% <= 0.607 milliseconds (cumulative count 99889)
99.997% <= 0.703 milliseconds (cumulative count 99997)
100.000% <= 0.807 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 50327.12 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.331     0.112     0.375     0.407     0.431     0.727
====== RPUSH ======                                                   
  100000 requests completed in 2.01 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 1)
50.000% <= 0.375 milliseconds (cumulative count 54255)
75.000% <= 0.391 milliseconds (cumulative count 83192)
87.500% <= 0.399 milliseconds (cumulative count 90911)
93.750% <= 0.407 milliseconds (cumulative count 94524)
96.875% <= 0.423 milliseconds (cumulative count 97100)
98.438% <= 0.455 milliseconds (cumulative count 98444)
99.219% <= 0.567 milliseconds (cumulative count 99236)
99.609% <= 0.615 milliseconds (cumulative count 99633)
99.805% <= 0.647 milliseconds (cumulative count 99816)
99.902% <= 0.671 milliseconds (cumulative count 99912)
99.951% <= 0.687 milliseconds (cumulative count 99971)
99.976% <= 0.695 milliseconds (cumulative count 99989)
99.994% <= 0.711 milliseconds (cumulative count 99994)
99.997% <= 0.735 milliseconds (cumulative count 99997)
99.998% <= 0.775 milliseconds (cumulative count 99999)
99.999% <= 0.895 milliseconds (cumulative count 100000)
100.000% <= 0.895 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
17.890% <= 0.207 milliseconds (cumulative count 17890)
21.903% <= 0.303 milliseconds (cumulative count 21903)
94.524% <= 0.407 milliseconds (cumulative count 94524)
98.940% <= 0.503 milliseconds (cumulative count 98940)
99.566% <= 0.607 milliseconds (cumulative count 99566)
99.993% <= 0.703 milliseconds (cumulative count 99993)
99.999% <= 0.807 milliseconds (cumulative count 99999)
100.000% <= 0.903 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 49800.80 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.336     0.112     0.375     0.415     0.535     0.895
====== LPOP ======                                                   
  100000 requests completed in 2.01 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 1)
50.000% <= 0.375 milliseconds (cumulative count 56078)
75.000% <= 0.391 milliseconds (cumulative count 81479)
87.500% <= 0.399 milliseconds (cumulative count 89147)
93.750% <= 0.415 milliseconds (cumulative count 94865)
96.875% <= 0.439 milliseconds (cumulative count 97149)
98.438% <= 0.479 milliseconds (cumulative count 98458)
99.219% <= 0.583 milliseconds (cumulative count 99220)
99.609% <= 0.631 milliseconds (cumulative count 99610)
99.805% <= 0.663 milliseconds (cumulative count 99832)
99.902% <= 0.679 milliseconds (cumulative count 99919)
99.951% <= 0.687 milliseconds (cumulative count 99964)
99.976% <= 0.695 milliseconds (cumulative count 99987)
99.988% <= 0.703 milliseconds (cumulative count 99994)
99.997% <= 0.735 milliseconds (cumulative count 99997)
99.998% <= 0.783 milliseconds (cumulative count 99999)
99.999% <= 0.807 milliseconds (cumulative count 100000)
100.000% <= 0.807 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
17.175% <= 0.207 milliseconds (cumulative count 17175)
22.070% <= 0.303 milliseconds (cumulative count 22070)
92.938% <= 0.407 milliseconds (cumulative count 92938)
98.601% <= 0.503 milliseconds (cumulative count 98601)
99.424% <= 0.607 milliseconds (cumulative count 99424)
99.994% <= 0.703 milliseconds (cumulative count 99994)
100.000% <= 0.807 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 49652.43 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.336     0.112     0.375     0.423     0.567     0.807
====== RPOP ======                                                   
  100000 requests completed in 2.00 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 2)
50.000% <= 0.375 milliseconds (cumulative count 56406)
75.000% <= 0.391 milliseconds (cumulative count 82764)
87.500% <= 0.399 milliseconds (cumulative count 90506)
93.750% <= 0.407 milliseconds (cumulative count 94150)
96.875% <= 0.431 milliseconds (cumulative count 97175)
98.438% <= 0.471 milliseconds (cumulative count 98485)
99.219% <= 0.575 milliseconds (cumulative count 99229)
99.609% <= 0.631 milliseconds (cumulative count 99645)
99.805% <= 0.663 milliseconds (cumulative count 99826)
99.902% <= 0.679 milliseconds (cumulative count 99910)
99.951% <= 0.695 milliseconds (cumulative count 99962)
99.976% <= 0.735 milliseconds (cumulative count 99977)
99.988% <= 0.799 milliseconds (cumulative count 99988)
99.994% <= 0.871 milliseconds (cumulative count 99994)
99.997% <= 0.895 milliseconds (cumulative count 99997)
99.998% <= 0.919 milliseconds (cumulative count 99999)
99.999% <= 0.935 milliseconds (cumulative count 100000)
100.000% <= 0.935 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
18.144% <= 0.207 milliseconds (cumulative count 18144)
22.280% <= 0.303 milliseconds (cumulative count 22280)
94.150% <= 0.407 milliseconds (cumulative count 94150)
98.764% <= 0.503 milliseconds (cumulative count 98764)
99.475% <= 0.607 milliseconds (cumulative count 99475)
99.972% <= 0.703 milliseconds (cumulative count 99972)
99.990% <= 0.807 milliseconds (cumulative count 99990)
99.997% <= 0.903 milliseconds (cumulative count 99997)
100.000% <= 1.007 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 49925.11 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.335     0.112     0.375     0.415     0.551     0.935
====== SADD ======                                                   
  100000 requests completed in 2.00 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.127 milliseconds (cumulative count 48)
50.000% <= 0.375 milliseconds (cumulative count 54743)
75.000% <= 0.391 milliseconds (cumulative count 87959)
93.750% <= 0.399 milliseconds (cumulative count 95031)
96.875% <= 0.407 milliseconds (cumulative count 97197)
98.438% <= 0.431 milliseconds (cumulative count 98785)
99.219% <= 0.455 milliseconds (cumulative count 99334)
99.609% <= 0.487 milliseconds (cumulative count 99620)
99.805% <= 0.583 milliseconds (cumulative count 99830)
99.902% <= 0.615 milliseconds (cumulative count 99909)
99.951% <= 0.639 milliseconds (cumulative count 99958)
99.976% <= 0.655 milliseconds (cumulative count 99984)
99.988% <= 0.663 milliseconds (cumulative count 99992)
99.994% <= 0.671 milliseconds (cumulative count 99994)
99.997% <= 0.687 milliseconds (cumulative count 99997)
99.998% <= 0.695 milliseconds (cumulative count 99999)
99.999% <= 0.703 milliseconds (cumulative count 100000)
100.000% <= 0.703 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
17.503% <= 0.207 milliseconds (cumulative count 17503)
22.357% <= 0.303 milliseconds (cumulative count 22357)
97.197% <= 0.407 milliseconds (cumulative count 97197)
99.646% <= 0.503 milliseconds (cumulative count 99646)
99.894% <= 0.607 milliseconds (cumulative count 99894)
100.000% <= 0.703 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 50125.31 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.332     0.120     0.375     0.399     0.439     0.703
====== HSET ======                                                   
  100000 requests completed in 2.04 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.127 milliseconds (cumulative count 28)
50.000% <= 0.383 milliseconds (cumulative count 61521)
75.000% <= 0.391 milliseconds (cumulative count 79137)
87.500% <= 0.399 milliseconds (cumulative count 90244)
93.750% <= 0.407 milliseconds (cumulative count 94890)
96.875% <= 0.423 milliseconds (cumulative count 97120)
98.438% <= 0.463 milliseconds (cumulative count 98538)
99.219% <= 0.583 milliseconds (cumulative count 99247)
99.609% <= 0.631 milliseconds (cumulative count 99657)
99.805% <= 0.655 milliseconds (cumulative count 99823)
99.902% <= 0.679 milliseconds (cumulative count 99940)
99.951% <= 0.687 milliseconds (cumulative count 99964)
99.976% <= 0.695 milliseconds (cumulative count 99982)
99.988% <= 0.703 milliseconds (cumulative count 99991)
99.994% <= 0.711 milliseconds (cumulative count 99994)
99.997% <= 0.743 milliseconds (cumulative count 99997)
99.998% <= 0.879 milliseconds (cumulative count 99999)
99.999% <= 0.895 milliseconds (cumulative count 100000)
100.000% <= 0.895 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
15.461% <= 0.207 milliseconds (cumulative count 15461)
20.300% <= 0.303 milliseconds (cumulative count 20300)
94.890% <= 0.407 milliseconds (cumulative count 94890)
98.819% <= 0.503 milliseconds (cumulative count 98819)
99.464% <= 0.607 milliseconds (cumulative count 99464)
99.991% <= 0.703 milliseconds (cumulative count 99991)
99.998% <= 0.807 milliseconds (cumulative count 99998)
100.000% <= 0.903 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 48923.68 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.343     0.120     0.383     0.415     0.551     0.895
====== SPOP ======                                                   
  100000 requests completed in 1.87 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 8)
50.000% <= 0.351 milliseconds (cumulative count 51326)
75.000% <= 0.383 milliseconds (cumulative count 75516)
87.500% <= 0.399 milliseconds (cumulative count 93463)
93.750% <= 0.407 milliseconds (cumulative count 95971)
96.875% <= 0.415 milliseconds (cumulative count 97011)
98.438% <= 0.447 milliseconds (cumulative count 98577)
99.219% <= 0.527 milliseconds (cumulative count 99224)
99.609% <= 0.599 milliseconds (cumulative count 99611)
99.805% <= 0.639 milliseconds (cumulative count 99843)
99.902% <= 0.655 milliseconds (cumulative count 99947)
99.951% <= 0.663 milliseconds (cumulative count 99972)
99.976% <= 0.671 milliseconds (cumulative count 99984)
99.988% <= 0.679 milliseconds (cumulative count 99994)
99.997% <= 0.695 milliseconds (cumulative count 99997)
99.998% <= 0.703 milliseconds (cumulative count 100000)
100.000% <= 0.703 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
22.077% <= 0.207 milliseconds (cumulative count 22077)
40.589% <= 0.303 milliseconds (cumulative count 40589)
95.971% <= 0.407 milliseconds (cumulative count 95971)
99.092% <= 0.503 milliseconds (cumulative count 99092)
99.651% <= 0.607 milliseconds (cumulative count 99651)
100.000% <= 0.703 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 53590.57 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.312     0.112     0.351     0.407     0.479     0.703
====== ZADD ======                                                   
  100000 requests completed in 2.06 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.127 milliseconds (cumulative count 24)
50.000% <= 0.383 milliseconds (cumulative count 57228)
75.000% <= 0.391 milliseconds (cumulative count 75921)
87.500% <= 0.399 milliseconds (cumulative count 88506)
93.750% <= 0.407 milliseconds (cumulative count 94083)
96.875% <= 0.431 milliseconds (cumulative count 97315)
98.438% <= 0.471 milliseconds (cumulative count 98585)
99.219% <= 0.583 milliseconds (cumulative count 99232)
99.609% <= 0.631 milliseconds (cumulative count 99619)
99.805% <= 0.663 milliseconds (cumulative count 99845)
99.902% <= 0.679 milliseconds (cumulative count 99916)
99.951% <= 0.687 milliseconds (cumulative count 99954)
99.976% <= 0.703 milliseconds (cumulative count 99977)
99.988% <= 0.751 milliseconds (cumulative count 99988)
99.994% <= 0.783 milliseconds (cumulative count 99995)
99.997% <= 0.887 milliseconds (cumulative count 99997)
99.998% <= 0.903 milliseconds (cumulative count 99999)
99.999% <= 0.911 milliseconds (cumulative count 100000)
100.000% <= 0.911 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
15.371% <= 0.207 milliseconds (cumulative count 15371)
19.410% <= 0.303 milliseconds (cumulative count 19410)
94.083% <= 0.407 milliseconds (cumulative count 94083)
98.830% <= 0.503 milliseconds (cumulative count 98830)
99.434% <= 0.607 milliseconds (cumulative count 99434)
99.977% <= 0.703 milliseconds (cumulative count 99977)
99.996% <= 0.807 milliseconds (cumulative count 99996)
99.999% <= 0.903 milliseconds (cumulative count 99999)
100.000% <= 1.007 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 48520.13 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.346     0.120     0.383     0.415     0.551     0.911
====== ZPOPMIN ======                                                   
  100000 requests completed in 2.01 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 18)
50.000% <= 0.375 milliseconds (cumulative count 57224)
75.000% <= 0.391 milliseconds (cumulative count 84835)
87.500% <= 0.399 milliseconds (cumulative count 91982)
93.750% <= 0.407 milliseconds (cumulative count 95130)
96.875% <= 0.423 milliseconds (cumulative count 97295)
98.438% <= 0.455 milliseconds (cumulative count 98527)
99.219% <= 0.551 milliseconds (cumulative count 99270)
99.609% <= 0.599 milliseconds (cumulative count 99611)
99.805% <= 0.639 milliseconds (cumulative count 99817)
99.902% <= 0.663 milliseconds (cumulative count 99917)
99.951% <= 0.679 milliseconds (cumulative count 99959)
99.976% <= 0.695 milliseconds (cumulative count 99987)
99.988% <= 0.703 milliseconds (cumulative count 99993)
99.994% <= 0.711 milliseconds (cumulative count 99995)
99.997% <= 0.719 milliseconds (cumulative count 99997)
99.998% <= 0.735 milliseconds (cumulative count 99999)
99.999% <= 0.743 milliseconds (cumulative count 100000)
100.000% <= 0.743 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
17.223% <= 0.207 milliseconds (cumulative count 17223)
22.027% <= 0.303 milliseconds (cumulative count 22027)
95.130% <= 0.407 milliseconds (cumulative count 95130)
99.024% <= 0.503 milliseconds (cumulative count 99024)
99.662% <= 0.607 milliseconds (cumulative count 99662)
99.993% <= 0.703 milliseconds (cumulative count 99993)
100.000% <= 0.807 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 49850.45 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.334     0.112     0.375     0.407     0.495     0.743
====== LPUSH (needed to benchmark LRANGE) ======                                                   
  100000 requests completed in 2.03 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.119 milliseconds (cumulative count 4)
50.000% <= 0.375 milliseconds (cumulative count 53738)
75.000% <= 0.391 milliseconds (cumulative count 81595)
87.500% <= 0.399 milliseconds (cumulative count 89109)
93.750% <= 0.415 milliseconds (cumulative count 94763)
96.875% <= 0.439 milliseconds (cumulative count 97121)
98.438% <= 0.487 milliseconds (cumulative count 98443)
99.219% <= 0.607 milliseconds (cumulative count 99268)
99.609% <= 0.647 milliseconds (cumulative count 99677)
99.805% <= 0.671 milliseconds (cumulative count 99842)
99.902% <= 0.687 milliseconds (cumulative count 99921)
99.951% <= 0.695 milliseconds (cumulative count 99952)
99.976% <= 0.711 milliseconds (cumulative count 99990)
99.994% <= 0.719 milliseconds (cumulative count 99996)
99.997% <= 0.727 milliseconds (cumulative count 99998)
99.998% <= 0.735 milliseconds (cumulative count 99999)
99.999% <= 0.799 milliseconds (cumulative count 100000)
100.000% <= 0.799 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
16.234% <= 0.207 milliseconds (cumulative count 16234)
21.013% <= 0.303 milliseconds (cumulative count 21013)
92.788% <= 0.407 milliseconds (cumulative count 92788)
98.514% <= 0.503 milliseconds (cumulative count 98514)
99.268% <= 0.607 milliseconds (cumulative count 99268)
99.975% <= 0.703 milliseconds (cumulative count 99975)
100.000% <= 0.807 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 49188.39 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.340     0.112     0.375     0.423     0.583     0.799
====== LRANGE_100 (first 100 elements) ======                                                   
  100000 requests completed in 7.97 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.623 milliseconds (cumulative count 1)
50.000% <= 1.495 milliseconds (cumulative count 50868)
75.000% <= 1.719 milliseconds (cumulative count 75234)
87.500% <= 1.735 milliseconds (cumulative count 98435)
98.438% <= 1.743 milliseconds (cumulative count 99871)
99.902% <= 1.751 milliseconds (cumulative count 99922)
99.951% <= 1.959 milliseconds (cumulative count 99955)
99.976% <= 1.991 milliseconds (cumulative count 99976)
99.988% <= 2.239 milliseconds (cumulative count 99990)
99.994% <= 2.255 milliseconds (cumulative count 99994)
99.997% <= 2.271 milliseconds (cumulative count 99997)
99.998% <= 2.503 milliseconds (cumulative count 99999)
99.999% <= 2.511 milliseconds (cumulative count 100000)
100.000% <= 2.511 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
0.001% <= 0.703 milliseconds (cumulative count 1)
0.002% <= 0.903 milliseconds (cumulative count 2)
0.008% <= 1.007 milliseconds (cumulative count 8)
0.012% <= 1.207 milliseconds (cumulative count 12)
0.085% <= 1.303 milliseconds (cumulative count 85)
0.095% <= 1.407 milliseconds (cumulative count 95)
62.652% <= 1.503 milliseconds (cumulative count 62652)
68.062% <= 1.607 milliseconds (cumulative count 68062)
68.558% <= 1.703 milliseconds (cumulative count 68558)
99.946% <= 1.807 milliseconds (cumulative count 99946)
99.950% <= 1.903 milliseconds (cumulative count 99950)
99.980% <= 2.007 milliseconds (cumulative count 99980)
100.000% <= 3.103 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 12545.48 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        1.566     0.616     1.495     1.735     1.743     2.511
====== LRANGE_300 (first 300 elements) ======                                                 
  100000 requests completed in 23.09 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 2.335 milliseconds (cumulative count 1)
50.000% <= 4.511 milliseconds (cumulative count 66978)
75.000% <= 4.527 milliseconds (cumulative count 96932)
98.438% <= 4.535 milliseconds (cumulative count 99741)
99.805% <= 4.543 milliseconds (cumulative count 99824)
99.902% <= 4.751 milliseconds (cumulative count 99921)
99.951% <= 4.759 milliseconds (cumulative count 99958)
99.976% <= 4.775 milliseconds (cumulative count 99984)
99.988% <= 4.815 milliseconds (cumulative count 99988)
99.994% <= 5.567 milliseconds (cumulative count 99994)
99.997% <= 6.415 milliseconds (cumulative count 99997)
99.998% <= 6.815 milliseconds (cumulative count 99999)
99.999% <= 7.095 milliseconds (cumulative count 100000)
100.000% <= 7.095 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
0.003% <= 3.103 milliseconds (cumulative count 3)
0.024% <= 4.103 milliseconds (cumulative count 24)
99.991% <= 5.103 milliseconds (cumulative count 99991)
99.996% <= 6.103 milliseconds (cumulative count 99996)
100.000% <= 7.103 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 4330.69 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        4.481     2.328     4.511     4.527     4.535     7.095
====== LRANGE_500 (first 500 elements) ======                                                 
  100000 requests completed in 38.93 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 2.279 milliseconds (cumulative count 1)
50.000% <= 7.527 milliseconds (cumulative count 59311)
75.000% <= 7.551 milliseconds (cumulative count 85786)
87.500% <= 7.559 milliseconds (cumulative count 99692)
99.805% <= 7.567 milliseconds (cumulative count 99920)
99.951% <= 7.735 milliseconds (cumulative count 99952)
99.976% <= 7.783 milliseconds (cumulative count 99982)
99.988% <= 7.791 milliseconds (cumulative count 99988)
99.994% <= 8.279 milliseconds (cumulative count 99994)
99.997% <= 9.055 milliseconds (cumulative count 99997)
99.998% <= 9.335 milliseconds (cumulative count 99999)
99.999% <= 9.831 milliseconds (cumulative count 100000)
100.000% <= 9.831 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
0.003% <= 3.103 milliseconds (cumulative count 3)
0.006% <= 4.103 milliseconds (cumulative count 6)
0.009% <= 5.103 milliseconds (cumulative count 9)
0.012% <= 6.103 milliseconds (cumulative count 12)
0.042% <= 7.103 milliseconds (cumulative count 42)
99.993% <= 8.103 milliseconds (cumulative count 99993)
99.998% <= 9.103 milliseconds (cumulative count 99998)
100.000% <= 10.103 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 2568.71 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        7.493     2.272     7.527     7.559     7.559     9.831
====== LRANGE_600 (first 600 elements) ======                                                 
  100000 requests completed in 46.13 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 1.599 milliseconds (cumulative count 1)
50.000% <= 8.815 milliseconds (cumulative count 58351)
75.000% <= 8.839 milliseconds (cumulative count 75980)
87.500% <= 9.039 milliseconds (cumulative count 91512)
93.750% <= 9.047 milliseconds (cumulative count 98111)
98.438% <= 9.055 milliseconds (cumulative count 99967)
99.976% <= 9.063 milliseconds (cumulative count 99983)
99.988% <= 9.079 milliseconds (cumulative count 99991)
99.994% <= 9.095 milliseconds (cumulative count 99994)
99.997% <= 9.911 milliseconds (cumulative count 99997)
99.998% <= 11.151 milliseconds (cumulative count 99999)
99.999% <= 11.327 milliseconds (cumulative count 100000)
100.000% <= 11.327 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
0.001% <= 1.607 milliseconds (cumulative count 1)
0.002% <= 1.903 milliseconds (cumulative count 2)
0.004% <= 3.103 milliseconds (cumulative count 4)
0.007% <= 4.103 milliseconds (cumulative count 7)
0.008% <= 5.103 milliseconds (cumulative count 8)
0.013% <= 6.103 milliseconds (cumulative count 13)
0.014% <= 7.103 milliseconds (cumulative count 14)
0.020% <= 8.103 milliseconds (cumulative count 20)
99.994% <= 9.103 milliseconds (cumulative count 99994)
99.997% <= 10.103 milliseconds (cumulative count 99997)
99.998% <= 11.103 milliseconds (cumulative count 99998)
100.000% <= 12.103 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 2167.83 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        8.858     1.592     8.815     9.047     9.055    11.327
====== MSET (10 keys) ======                                                   
  100000 requests completed in 3.44 seconds
  20 parallel clients
  3 bytes payload
  keep alive: 1
  host configuration "save": 3600 1 300 100 60 10000
  host configuration "appendonly": no
  multi-thread: no

Latency by percentile distribution:
0.000% <= 0.231 milliseconds (cumulative count 2)
50.000% <= 0.711 milliseconds (cumulative count 81756)
87.500% <= 0.719 milliseconds (cumulative count 96241)
96.875% <= 0.727 milliseconds (cumulative count 98913)
99.219% <= 0.735 milliseconds (cumulative count 99565)
99.609% <= 0.743 milliseconds (cumulative count 99659)
99.805% <= 0.847 milliseconds (cumulative count 99807)
99.902% <= 0.983 milliseconds (cumulative count 99913)
99.951% <= 1.015 milliseconds (cumulative count 99953)
99.976% <= 1.055 milliseconds (cumulative count 99979)
99.988% <= 1.199 milliseconds (cumulative count 99989)
99.994% <= 1.295 milliseconds (cumulative count 99994)
99.997% <= 1.359 milliseconds (cumulative count 99997)
99.998% <= 1.575 milliseconds (cumulative count 99999)
99.999% <= 1.591 milliseconds (cumulative count 100000)
100.000% <= 1.591 milliseconds (cumulative count 100000)

Cumulative distribution of latencies:
0.000% <= 0.103 milliseconds (cumulative count 0)
0.005% <= 0.303 milliseconds (cumulative count 5)
0.026% <= 0.407 milliseconds (cumulative count 26)
1.223% <= 0.503 milliseconds (cumulative count 1223)
27.187% <= 0.607 milliseconds (cumulative count 27187)
49.688% <= 0.703 milliseconds (cumulative count 49688)
99.801% <= 0.807 milliseconds (cumulative count 99801)
99.815% <= 0.903 milliseconds (cumulative count 99815)
99.944% <= 1.007 milliseconds (cumulative count 99944)
99.982% <= 1.103 milliseconds (cumulative count 99982)
99.989% <= 1.207 milliseconds (cumulative count 99989)
99.994% <= 1.303 milliseconds (cumulative count 99994)
99.997% <= 1.407 milliseconds (cumulative count 99997)
100.000% <= 1.607 milliseconds (cumulative count 100000)

Summary:
  throughput summary: 29036.00 requests per second
  latency summary (msec):
          avg       min       p50       p95       p99       max
        0.657     0.224     0.711     0.719     0.735     1.591
'''

redisLines = redisOutput.split('\n')

result = []
curReportName = None
for line in redisLines:
    if line.startswith('======'):
        line = line.strip()
        testName = line[7:len(line) - 7]
        result.append([testName, 0])
    elif line.startswith('  throughput summary:'):
        line = line.strip()
        # print(line.split())
        result[-1][1] = line.split()[2]
print(result)

result = sorted(result, key=lambda x: x[0])

for k, v in result:
    print('%s\t%s' % (k, v))
