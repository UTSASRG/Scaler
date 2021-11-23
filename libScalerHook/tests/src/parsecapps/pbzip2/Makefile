HOME = ../..
include ../defines.mk

TEST_NAME = pbzip2
TEST_FILES = pbzip2 BZ2StreamScanner ErrorContext

TEST_ARGS = -p$(NCORES) -r -5 -k -f ../../datasets/pbzip2/gcc-3.3.tar.gz-1 ../../datasets/pbzip2/gcc-3.3.tar.gz-2 ../../datasets/pbzip2/gcc-3.3.tar.gz-3 ../../datasets/pbzip2/gcc-3.3.tar.gz-4 ../../datasets/pbzip2/gcc-3.3.tar.gz-5 ../../datasets/pbzip2/gcc-3.3.tar.gz-6 ../../datasets/pbzip2/gcc-3.3.tar.gz-7 ../../datasets/pbzip2/gcc-3.3.tar.gz-8 ../../datasets/pbzip2/gcc-3.3.tar.gz-9 ../../datasets/pbzip2/gcc-3.3.tar.gz-10

CFLAGS = -L./library/bzip2-1.0.6/install/lib -I./library/bzip2-1.0.6/install/include -DPBZIP_DEBUG -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_POSIX_PTHREAD_SEMANTICS -DUSE_STACKSIZE_CUSTOMIZATION
LIBS = -lstdc++ -lbz2 -lpthread

include $(HOME)/Default.mk

