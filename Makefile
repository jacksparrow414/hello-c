# Windows下把/缓存\

objects = helloWorld.o struct_learn/book.o
hello : $(objects)
	cc -o hello $(objects)
helloWorld.o : struct_learn/book.h
.PHONY : clean
clean :
# Windows下把rm换成del https://stackoverflow.com/questions/47735464/make-clean-not-working
	-rm hello $(objects)