####Prerequisites

For general Android prerequisites, see http://source.android.com/

1. Download the whole Android code tree from source.android.com, use the TAG android-2.2.1_r1

2. Suppose the work directory of step #1 is /home/prj/

   export MYPKGTOP=/home/prj
   cd ${MYPKGTOP}

3. decompress modules to your work directory

  bionic.tar.gz
  build.tar.gz
  dalvik.tar.gz
  external.tar.gz
  kernel.tar.gz
   
4. Build
   Run "make"
   