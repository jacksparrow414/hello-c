[toc]
# hello-c

Learn C Language Project

# what is the fundamental difference between source and header files in c? 
[what-is-the-fundamental-difference-between-source-and-header-files-in-c](https://stackoverflow.com/questions/3482948/what-is-the-fundamental-difference-between-source-and-header-files-in-c)

# compling multiple C files using gcc

- [how-do-i-compile-multiple-c-files-into-one-executable](https://stackoverflow.com/questions/47073126/how-do-i-compile-multiple-c-files-into-one-executable)
- [compiling-multiple-c-files-in-a-program](https://stackoverflow.com/questions/8728728/compiling-multiple-c-files-in-a-program)

example 1
```shell
gcc copy.c delete.c extension.c list.c your_app.c -o you_app
```

example 2
```shell
gcc struct_learn/book.c helloWorld.c -o helloWorld.exe
```

example 3
write a [makefile](#how-to-write-makefile?), then run
```shell
make
```
# C Standard Library Quick Reference
- https://en.cppreference.com/w/c/header

# how to write makefile?
[how-to-write-makefile](https://seisman.github.io/how-to-write-makefile/overview.html)

# recommended books
- The C Programming Language(Second Edition)
