HOME = ../..

include $(HOME)/tests/defines.mk
TEST_NAME = aget
TEST_FILES = Download Aget Head main Resume Signal Misc

#TEST_ARGS = -f -n $(NCORES) http://www.kernel.org/pub/linux/kernel/v2.6/linux-2.6.38.tar.gz
#TEST_ARGS = -f -n 8 http://ftp.gnu.org/gnu/glibc/glibc-2.18.tar.xz
TEST_ARGS = -f -n 20  http://ftp.gnu.org/gnu/gcc/gcc-3.3/gcc-3.3.tar.gz
#TEST_ARGS = -f -n $(NCORES) http://gnu.mirrors.hoobly.com/gnu/gcc/gcc-3.3.tar.gz
#TEST_ARGS = -f -n 8 http://mirror.csclub.uwaterloo.ca/gnu/gcc/gcc-2.95.1.tar.gzz
#http://www.kernel.org/pub/linux/kernel/v2.6/linux-2.6.38.tar.gz

CFLAGS = -DCATCH_SIGNALS
LIBS = -lm -lstdc++

DMP_O_GRAN = 6
DMP_O_QUANTUM = 100000

DMP_B_GRAN = 6
DMP_B_QUANTUM = 100000

include $(HOME)/Default.mk

