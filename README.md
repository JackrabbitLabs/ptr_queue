# Overview

This is a C library that implements a basic queue. This queue utilizes a 
global mutex and is multi-writer, multi-reader thread safe. 

# Supported Operating System Versions

- Ubuntu 23.10
- Fedora 38, 39

# Building

1. Install OS libraries

Install the following build packages to compile the software on the following
operating systems.

**Ubuntu:**

```bash
apt install build-essential 
```

**Fedora:**

```bash
```

2. Build Dependencies

This library does not depend upon any other non-os provided library. 

3. Build

To build, simply run 

```bash
make
```

