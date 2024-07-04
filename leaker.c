/*
 * tiSample.c
 *
 * Sample JVMTI agent to demonstrate the IBM JVMTI dump extensions
 */

#ifndef _WIN32
#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>
#else
#include <Windows.h>
#endif

#include "jvmti.h"

#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>

#define LOG(s)  { printf s; printf("\n"); fflush(stdout); }

volatile void* p1 = NULL;
volatile void* p2 = NULL;

static bool env_var_true(const char* var) {
	const char* s = getenv(var);
	if (s != NULL) {
//		LOG(("%s=%s", var, s));
		if (std::strcmp(s, "true") == 0 || std::strcmp(s, "1") == 0) return true;
		if (std::strcmp(s, "false") == 0 || std::strcmp(s, "0") == 0) return false;
		assert(0);
	}
	return false;
}	

static bool should_leak_mmap() {
	return env_var_true("LEAK_MMAP");
}

static bool should_leak_malloc() {
	return env_var_true("LEAK_MALLOC");
}

static bool use_dlsym() {
	return env_var_true("DL_SYM");
}

typedef void* (*malloc_type_t)(size_t s);
static malloc_type_t g_malloc = NULL;

static void do_malloc() {
	size_t l = rand() % (1024 * 1024); 
#ifndef _WIN32
	if (use_dlsym()) {
		if (g_malloc == NULL) {
			g_malloc = dlsym(RTLD_DEFAULT, "malloc");
		}
		p1 = g_malloc(l);
		LOG(("leak malloc (dyn) %p", p1));
	} else {
		p1 = malloc(l);
		LOG(("leak malloc (stat) %p", p1));
	}
#else
	p1 = malloc(l);
	LOG(("leak malloc (stat) %p", p1));
#endif
}

static void leak_malloc() {
	if (should_leak_malloc()) {
		do_malloc();
	}
}

static void leak_mmap() {
	if (should_leak_mmap()) {
#ifndef _WIN32
		void* p = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#else
		void* p = VirtualAlloc(NULL, 4096, MEM_RESERVE, PAGE_READWRITE);
		void* q = VirtualAlloc(p, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#endif
		LOG(("leak mmap %p", p));
	}
}

static void leakleak() {
	leak_malloc();
	leak_mmap();
}

static void leakleakleak() {
	leakleak();
}

static void leakabit() {
	leakleakleak();
}

int x = 1000;
int y = 1000;

#ifdef _WIN32
__declspec(noinline) 
#endif
void bad_function() {
  char* buffer = (char*)malloc(x * y * x * y); //Boom!
  memcpy(buffer, buffer + 8, 8); 
}


static void* leaky_thread() {
	for (;;) {
		leakabit();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

static void* greedy_thread() {
	for (;;) {
		bad_function();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}


/* A thread that is unknown to the JVM */
static void start_leaky_thread() {
#ifndef _WIN32
	int rc = 0;
	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	rc = pthread_create(&tid, &attr, leaky_thread, NULL);
	if (rc == 0) {
		LOG(("started leaky thread"));
	} else {
		LOG(("thread start failure %d", errno));
	}
#else
	std::thread tid(leaky_thread);
	LOG(("started leaky thread"));
#endif
}

extern "C" int main(int argc, char* argv[]) {
	LOG(("Loading main\n"));
	LOG(("Compiled %s %s", __DATE__, __TIME__));
	bad_function();
	start_leaky_thread();
	return 0;
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {

	jvmtiEnv *jvmti = NULL;
	jvmtiError rc;
	int i = 0, j = 0;

	LOG(("Loading JVMTI sample agent\n"));
	LOG(("Compiled %s %s", __DATE__, __TIME__));

	std::thread greedy(greedy_thread);
	LOG(("started greedy thread"));
			std::this_thread::sleep_for(std::chrono::seconds(3));

	start_leaky_thread();

	// (*jvm)->GetEnv(jvm, (void **)&jvmti, JVMTI_VERSION_1_0);

    return JNI_OK;
}

