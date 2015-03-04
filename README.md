# bam-window
Count BAM reads in windows by library.

## Build Requirements
- gcc 4.4+ (or equivalent clang version)
- cmake 2.8+

## Build instructions

From the top level of the repo:

```
mkdir build
cd build
cmake ..
make
make install #optional, installs to /usr by default
```

To change the installation location, change the cmake command to:

```
cmake .. -DCMAKE_INSTALL_PREFIX=/somewhere/else
```

If the install step is skipped, the executable will be located at $REPO/build/bin/bam-window.
