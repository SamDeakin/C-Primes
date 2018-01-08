# CPPrimes
Implements a simple Sieve of Eratosthenes-like algorithm concurrently to find small prime numbers.

## Building
First sync git submodules

`git submodule update --init`

Create a build directory

`mkdir BUILD && cd BUILD`

Generate build files with CMake

`cmake ..`

Build the executable with make

`make CPPrimes`

## Command Line Arguments
`-t <n>` Run with `n` threads. Default 2

`-s <n>` Find numbers starting at `n`. Default 2

`-e <n>` Find numbers ending at `n`. Default 100
