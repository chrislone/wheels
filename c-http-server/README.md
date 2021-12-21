## A simple http server

### Usage

```shell
$ make
$ ./main 127.0.0.1 8890
```

use curl to get response:

```shell
$ curl 127.0.0.1:8890
# <!DOCTYPE html>
# <html><body><div>You are using GET method.</div></body></html>
```

```shell
$ curl -X WHATEVER 127.0.0.1:8890
# <!DOCTYPE html>
# <html><body><div>You are using WHATEVER method.</div></body></html>
```
