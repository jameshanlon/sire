# Sire pre

An early Sire language compiler targeting the XMOS XS1 architecture.

Prerequisites:
- Flex
- Bison >=3.2

To build:
```
$ make
```

To compile a program:
```
$ ./bin/sire tests/factorial.x
$ cat program.S
...
```

View the AST:
```
$ ./bin/sire -ast tests/factorial.x
func factorial (x: int) is
    if (x = 0)
    then
        return 1
    else
        return (x * factorial((x - 1)))

proc main () is
    factorial(5)
```

View the IR:
```
$ ./bin/sire -ir tests/factorial.x
proc factorial (val x) {
  
  .L5:
    .T0 := x = 0
    cjump .T0 .L1 .L2
  
  .L2:
    .T1 := x - 1
    .T2 := fcall factorial (.T1)
    return .L0 x * .T2
  
  .L1:
    return .L0 1
  
  .L0:
    end
}

proc _main () {
  
  .L7:
    .T0 := 5
    pcall factorial (.T0)
  
  .L4:
    end
}
```
