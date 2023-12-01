/*
 * tiSample.c
 *
 * Sample JVMTI agent to demonstrate the IBM JVMTI dump extensions
 */

#define _GNU_SOURCE 

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "jvmti.h"
#include <sys/mman.h>
#include <stdbool.h>
#include <strings.h>
#include <assert.h>


#define LOG(s)  { printf s; printf("\n"); fflush(stdout); }

volatile void* p1 = NULL;
volatile void* p2 = NULL;

static bool env_var_true(const char* var) {
	const char* s = getenv(var);
	if (s != NULL) {
//		LOG(("%s=%s", var, s));
		if (strcasecmp(s, "true") == 0 || strcasecmp(s, "1") == 0) return true;
		if (strcasecmp(s, "false") == 0 || strcasecmp(s, "0") == 0) return false;
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
}

static void leak_malloc() {
	if (should_leak_malloc()) {
		do_malloc();
	}
}

static void leak_mmap() {
	if (should_leak_mmap()) {
		void* p = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
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

static void* leaky_thread(void* dummy) {
	for (;;) {
		leakabit();
		sleep(1);
	}
}

/* A thread that is unknown to the JVM */
static void start_leaky_thread() {
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
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {

	jvmtiEnv *jvmti = NULL;
	jvmtiError rc;
	int i = 0, j = 0;

	LOG(("Loading JVMTI sample agent\n"));
	LOG(("Compiled %s %s", __DATE__, __TIME__));

	start_leaky_thread();

	// (*jvm)->GetEnv(jvm, (void **)&jvmti, JVMTI_VERSION_1_0);

     	return JNI_OK;

}

