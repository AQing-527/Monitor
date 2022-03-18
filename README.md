# Monitor
A system program - monitor, which is being used by end-users to obtain the execution statistics of a program or sequence of programs connected by pipes. 

## Environment: Linux

### hen invoking the monitor program without arguments, it will terminate successfully without any output.

### When invoking the monitor program with input arguments, it will interpret the input and execute the corresponding program(s) with necessary arguments.
Examples:

− absolute path (starting with /) specified in the command line, e.g.

```
./monitor /home/tmchan/a.out
```

− relative path (starting with .) specified in the command line, e.g.

```
./monitor ./a.out abc 1000
```

### When the invoked program is terminated, the monitor process will print out the wall clock (real) time, user time, and system time used by that process as well as the number of page faults and context switches experienced by that process.

### The monitor program uses a special symbol “!” to act as the pipe operator. When the monitor process detects that its input arguments contain the special symbol “!”, it will identify the sequence of commands and start the corresponding programs with necessary arguments. In addition, these programs will be linked up according to the logical pipe defined by the input arguments. 
Example:

```
./monitor cat c3230a.txt ! grep kernel ! wc -w
```
