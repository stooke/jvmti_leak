set -e

gcc -c -fPIC -I /shared/projects/openjdk/jdks/openjdk11/include/ -I /shared/projects/openjdk/jdks/openjdk11/include/linux/ -o leaker.o leaker.c
gcc -shared -fPIC -o leaker.so leaker.o -lc

# call with: java -agentpath:<path to leaker.so>
