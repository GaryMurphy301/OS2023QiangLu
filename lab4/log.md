### A SIGSEGV (segmentation fault) is firing in malloc

![image-20231203192310007](C:/Users/lemon/AppData/Roaming/Typora/typora-user-images/image-20231203192310007.png)

```
➜  http_load-09Mar2016 ./http_load -parallel 10 -fetches 1000 urls.txt
1000 fetches, 10 max parallel, 260000 bytes, in 0.052216 seconds
260 mean bytes/connection
19151.2 fetches/sec, 4.97932e+06 bytes/sec
msecs/connect: 0.087026 mean, 0.7 max, 0.014 min
msecs/first-response: 0.22475 mean, 3.743 max, 0.042 min
HTTP response codes:
  code 200 -- 1000

```

```
➜  http_load-09Mar2016 ./http_load -parallel 10 -fetches 1000 urls.txt
1000 fetches, 10 max parallel, 260000 bytes, in 0.076752 seconds
260 mean bytes/connection
13029 fetches/sec, 3.38753e+06 bytes/sec
msecs/connect: 0.124987 mean, 3.992 max, 0.014 min
msecs/first-response: 0.289556 mean, 2.323 max, 0.045 min
HTTP response codes:
  code 200 -- 1000
➜  http_load-09Mar20
```

```
➜  http_load-09Mar2016 ./http_load -parallel 10 -fetches 1000 urls.txt
1000 fetches, 10 max parallel, 260000 bytes, in 0.071381 seconds
260 mean bytes/connection
14009.3 fetches/sec, 3.64243e+06 bytes/sec
msecs/connect: 0.134086 mean, 1.116 max, 0.013 min
msecs/first-response: 0.323018 mean, 1.884 max, 0.072 min
HTTP response codes:
  code 200 -- 1000

```

```
➜  http_load-09Mar2016 ./http_load -parallel 10 -fetches 1000 urls.txt
1000 fetches, 10 max parallel, 260000 bytes, in 0.078894 seconds
260 mean bytes/connection
12675.2 fetches/sec, 3.29556e+06 bytes/sec
msecs/connect: 0.142843 mean, 2.582 max, 0.013 min
msecs/first-response: 0.303359 mean, 2.441 max, 0.078 min
HTTP response codes:
  code 200 -- 1000

```

```
➜  http_load-09Mar2016 ./http_load -parallel 10 -fetches 1000 urls.txt
1000 fetches, 10 max parallel, 260000 bytes, in 0.071919 seconds
260 mean bytes/connection
13904.5 fetches/sec, 3.61518e+06 bytes/sec
msecs/connect: 0.129232 mean, 0.681 max, 0.012 min
msecs/first-response: 0.298131 mean, 6.035 max, 0.046 min
HTTP response codes:
  code 200 -- 1000

```

```
➜  http_load-09Mar2016 ./http_load -parallel 10 -fetches 1000 urls.txt
1000 fetches, 10 max parallel, 260000 bytes, in 0.075317 seconds
260 mean bytes/connection
13277.2 fetches/sec, 3.45208e+06 bytes/sec
msecs/connect: 0.144974 mean, 1.523 max, 0.013 min
msecs/first-response: 0.283606 mean, 2.968 max, 0.037 min
HTTP response codes:
  code 200 -- 1000

```

