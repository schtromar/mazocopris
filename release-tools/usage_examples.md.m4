m4_changequote(`[[[', `]]]')m4_dnl
Run as a simple server on port 8080, perform no text conversion, output
received data to the serial port and exit after one connection. Note
that superuser privileges are required if the specified port is smaller
than 1024.

```
copris -p 8080 /dev/ttyS0
```

Serve on port 8080 as a daemon (do not exit after first connection),
translate characters using the `slovene.ini` translation file, limit
any incoming text to maximum 100 characters and print received data to
the terminal. Note that text limit works only when running as a server.

```
copris -p 8080 -d -t slovene.ini -l 100
```

Read local file `Manual.md` using the specified printer feature set
`epson.ini` and output formatted text to a USB interface on the local
computer:

```
copris -r epson.ini /dev/ttyUSB0 < Manual.md
```
