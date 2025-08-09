# LBXplode

[LBXplode](http://burningsmell.org/lbxplode/) is a extremely simple app for dumping the contents of Master of Orion I/II .lbx files. It also auto-identifies contained WAV files and gives them the correct file extension. It performs endian conversion from the files' little-endian format, so theoretically this should work on big-endian platforms, not just its native one. Usage is extremely simple:

```sh
lbxplode file1 file2 ... fileN
```

The original code was written in October 2003 by Tyler Montbriand, [monttyle@heavyspace.ca](mailto:monttyle@heavyspace.ca).

Please direct any bug reports with this fork to the repository owner.
