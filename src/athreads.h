// thx to luminance for making dis
// https://github.com/unknownopponent/C_Simple_Tread


#pragma once

#include <assert.h>
#include <string.h> // memset
#include <windows.h>


typedef struct A_Thread {
	void(*function);
	void* args;
	HANDLE thread_handle;

} A_Thread;


char ath_create(A_Thread* thread);
char ath_join(A_Thread* thread, int* return_code);

void ath_exit(int ret);


inline char ath_create(A_Thread* thread) {

	assert(thread->function != 0);

	thread->thread_handle = CreateThread(0, 0,
								(LPTHREAD_START_ROUTINE)thread->function,
								thread->args, 0, 0);

	if (thread->thread_handle) {
		return 0;
    }

	return 1;
}


inline char ath_join(A_Thread* thread, int* return_code) {

	assert(thread->thread_handle);

	DWORD res = WaitForSingleObject(thread->thread_handle, INFINITE);
	if (res) {
		assert(0);
		return 1;
	}

	DWORD ret;
	BOOL res2 = GetExitCodeThread(thread->thread_handle, &ret);
	if (!res2) {
		assert(0);
		return 1;
	}

	res2 = CloseHandle(thread->thread_handle);
	if (res) {
		assert(0);
		return 1;
	}

	memset(&thread->thread_handle, '\0', sizeof(HANDLE));
	*return_code = (int)ret;

	return 0;
}


inline void ath_exit(int ret) {
	ExitThread(ret);
}