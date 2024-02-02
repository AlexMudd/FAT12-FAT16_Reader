all:
	@echo 'Usage: '
	@echo 'build - to compile program and create folder for files'
	@echo 'run12 - to run FAT12 Reader'
	@echo 'run16 - to run FAT16 Reader'
	@echo 'clean - to clear image files folder and remove executable file'
build:
	gcc fat12.c -o fat12
	gcc fat16.c -o fat16
	mkdir files
	
run12:
	@./fat12
	
run16:
	@./fat16

clean:
	@rm -rf files	
	@rm fat12
	@rm fat16
