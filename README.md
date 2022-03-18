# Sire pre

An early Sire language compiler targeting the XMOS XS1 architecture.

To build:
```
$ make
```

To compile a program:
```
$ ./bin/sire tests/bubblesort.x -o a.S
```

View the AST:
```
$ ./bin/sire -ast tests/bubblesort.x
```

View the IR:
```
$ ./bin/sire -ir tests/bubblesort.x
```
