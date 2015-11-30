# Android-Inline-Hook
thumb16 thumb32 arm32 inlineHook in Android

# Build
```ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk```

# Example
```C
#include <stdio.h>
#include <pthread.h>

int new_a()
{
	printf("new_a\n");
}

int a()
{
	printf("a\n");
}

int thread()
{
	while(1) {
		a();
	}
}

int (*old_a)() = NULL;

int main()
{
	int err;
	int i;
	pthread_t tid[10];

	for (i = 0; i < 1; ++i) {
		err = pthread_create(&tid[i], NULL, thread, NULL);
		if (err) {
			return -1;
		}
	}

	sleep(1);
	
	registerInlineHookByAddr(a, new_a, &old_a);
	while(inlineHook() < 0);
	sleep(5);
	unregisterInlineHookByAddr(a);
	while(inlineUnHook() < 0);

	for (i = 0; i < 1; ++i) {
		pthread_join(tid[i], NULL);
	}

	return 0;
}
```

# Contact
If you find any bugs, please contact me(ele7enxxh@qq.com)
