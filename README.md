# Android-Inline-Hook
thumb16 thumb32 arm32 inlineHook in Android

# Build
```ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk```

# Example
```C
int (*old_puts)(char *) = NULL;

int my_puts(char *str)
{
	old_puts(str);
	printf("secauo, %s\n", str);
}

int main()
{
	inlineHookByName("puts", "libc.so", 0, (uint32_t) my_puts, (uint32_t **) &old_puts);
	
	printf("seven\n");
	
	inlineUnHookByName("puts", "libc.so");
	
	printf("seven\n");
}
```

# Contact
If you find any bugs, please contact me(ele7enxxh@qq.com)
