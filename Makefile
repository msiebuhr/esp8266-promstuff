all: format

format:
	clang-format -i *.h*
	clang-format -assume-filename=Cpp -i *.ino
