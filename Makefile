# Windows下把/换成\
# Windows下使用mingw32-make

objects = helloWorld.o struct_learn/book.o input_output_learn/read_user_input.o file_socket_learn/file_example.o
hello : $(objects)
	cc -o hello $(objects)
helloWorld.o : struct_learn/book.h input_output_learn/read_user_input.h file_socket_learn/file_example.h
.PHONY : clean
clean :
# Windows下把rm换成del https://stackoverflow.com/questions/47735464/make-clean-not-working
	-rm hello $(objects)