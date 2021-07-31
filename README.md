# Kernel INI Parser
___

#### Preview:

![Preview](https://i.imgur.com/eYGjO0O.png)

####
####
##### Usage Example:

```c
SINIParser* pINI = INIParser("C:\\Driver\\test.ini");
if (pINI)
{
	DbgPrint("%s (bool) - %i", PRINT_PREFIX, INIParser_GetBool(pINI, "bool", FALSE));
	DbgPrint("%s (int) - %i", PRINT_PREFIX, INIParser_GetInt(pINI, "int", -1));
	DbgPrint("%s (int_hex) - %i", PRINT_PREFIX, INIParser_GetInt(pINI, "int_hex", -1));
	DbgPrint("%s (hex) - %x", PRINT_PREFIX, INIParser_GetUnsignedInt(pINI, "hex", 0U));

	INIParser_Free(pINI);
}
```