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
	struct inlineHookInfo info;

	memset(&info, 0, sizeof(info));

	char *so_name = "/system/lib/libc.so";
	char *symbol_name = "puts";

	strncpy(info.so_name, so_name, strlen(so_name));
	strncpy(info.symbol_name, symbol_name, strlen(symbol_name));
	info.new_addr =(uint32_t) my_puts;

	inlineHook(&info);

	old_puts = info.trampoline_instructions;

	printf("seven\n");
	
	inlineUnHook(&info);
}
```

# Contact
If you find any bugs, please contact me(197415663@qq.com)
