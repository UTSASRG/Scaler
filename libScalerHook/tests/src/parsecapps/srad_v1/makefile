# Example
# target: dependencies
	# command 1
	# command 2
          # .
          # .
          # .
	# command n

# link objects(binaries) together
a.out:	main.o
	gcc	main.o 	-O2 -L /media/umass/datasystem/steven/Scaler/cmake-build-release/libScalerHook -l ScalerHook-HookAuto -lm -g -fopenmp -o srad

# compile main function file into object (binary)
main.o: 	main.c define.c graphics.c
	gcc	main.c -c -L /media/umass/datasystem/steven/Scaler/cmake-build-release/libScalerHook -l ScalerHook-HookAuto -lm -g -O2 -fopenmp

# delete all object files
clean:
	rm *.o srad
