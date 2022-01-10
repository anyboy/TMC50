#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
	FILE *in_symbols, *out_apis;
	char line_char[1024];
	int line_len;
	int lines = 0;
	int i, j;
	int line_head_flag;
	unsigned int symbol_addr, hex_char_value;
	char symbol_type[32];
	int symbol_type_len;
	char global_type[32];
	int global_type_len;
	char filter_string[32];
	char filter_string_len;
	int mips16_flag;
	char symbol_name[128];
	char symbol_name_len;
	char *symbols_txt;
	char *export_apis_h;

	if (argc != 3)
	{
		printf("argc != 3\n");
		return 0;
	}
	
	symbols_txt = argv[1];
	export_apis_h = argv[2];
	
	in_symbols = fopen(symbols_txt, "r");
	if (in_symbols == NULL)
	{
		printf("Not find [%s]!\n", symbols_txt);
		return 0;
	}
	
	out_apis = fopen(export_apis_h, "w");
	if (out_apis == NULL)
	{
		printf("Create [%s] Fail!\n", export_apis_h);
		fclose(in_symbols);
		return 0;
	}
	
	while (1)
	{
		if (fgets(line_char, 1023, in_symbols) == NULL)
		{
			break;
		}
		
		line_len = strlen(line_char);
		
		//find number: , it means the Beginning of symbol item
		line_head_flag = 1;
		for (i = 0; i < line_len; i++)
		{
			if (line_char[i] >= '0' && line_char[i] <= '9')
			{
				line_head_flag = 2;
				continue;
			}
			if (line_char[i] == ':')
			{
				if (line_head_flag == 2)
				{
					i++;
					line_head_flag = 3;
					break;
				}
				else
				{
					line_head_flag = 0;
				}
			}
			else if (line_char[i] == ' ')
			{
				continue;
			}
			else
			{
				line_head_flag = 0;
			}

			if (line_head_flag == 0)
				break;
		}
		
		if (line_head_flag == 3)
		{
			for (; i < line_len; i++)
			{
				if (line_char[i] != ' ')
				{
					break;
				}
			}

			//Read in a hexadecimal number of eight lowercase characters
			symbol_addr = 0;
			for (j = 0; i < line_len && j < 8; i++, j++)
			{
				if (line_char[i] >= 'a')
				{
					hex_char_value = line_char[i] - 'a' + 10;
				}
				else
				{
					hex_char_value = line_char[i] - '0';
				}
				
				symbol_addr = symbol_addr<<4;
				symbol_addr += hex_char_value;
			}
			
		
			for (; i < line_len; i++)
			{
				if (line_char[i] != ' ')
				{
					break;
				}
			}

			//Filter out a decimal number
			for (; i < line_len; i++)
			{
				if (line_char[i] >= '0' && line_char[i] <= '9')
				{
					continue;
				}
				else
				{
					break;
				}
			}
			
			for (; i < line_len; i++)
			{
				if (line_char[i] != ' ')
				{
					break;
				}
			}
			
			//Get symbol type FUNC OBJECT
			symbol_type_len = 0;
			for (; i < line_len; i++)
			{
				if (line_char[i] != ' ')
				{
					symbol_type[symbol_type_len++] = line_char[i];
				}
				else
				{
					break;
				}
			}
			symbol_type[symbol_type_len] = 0x0;

			for (; i < line_len; i++)
			{
				if (line_char[i] != ' ')
				{
					break;
				}
			}

			//GLOBAL or LOCAL
			global_type_len = 0;
			for (; i < line_len; i++)
			{
				if (line_char[i] != ' ')
				{
					global_type[global_type_len++] = line_char[i];
				}
				else
				{
					break;
				}
			}
			global_type[global_type_len] = 0x0;

			if (strcmp(global_type, "GLOBAL") == 0)
			{
				if ((strcmp(symbol_type, "FUNC") != 0) && (strcmp(symbol_type, "OBJECT") != 0) && (strcmp(symbol_type, "NOTYPE") != 0))
				{
					continue;
				}

				//filter out DEFAULT
				for (; i < line_len; i++)
				{
					if (line_char[i] != ' ')
					{
						break;
					}
				}

				//GLOBAL or LOCAL
				filter_string_len = 0;
				for (; i < line_len; i++)
				{
					if (line_char[i] != ' ')
					{
						filter_string[filter_string_len++] = line_char[i];
					}
					else
					{
						break;
					}
				}
				filter_string[filter_string_len] = 0x0;
				
				for (; i < line_len; i++)
				{
					if (line_char[i] != ' ')
					{
						break;
					}
				}
				
				//FUNC if is MIPS16 ?
				mips16_flag = 0;
				if (strcmp(symbol_type, "FUNC") == 0)
				{
					if (memcmp(&line_char[i], "[MIPS16]", 8) == 0)
					{
						mips16_flag = 1;
						i += 8;
						//printf("is mips16\n");
					}
				}
				
				for (; i < line_len; i++)
				{
					if (line_char[i] != ' ')
					{
						break;
					}
				}
				
				if (memcmp(&line_char[i], "ABS", 3) == 0)
				{
					//printf("is abs\n");
					continue;
				}
				
				for (; i < line_len; i++)
				{
					if (line_char[i] != ' ')
					{
						break;
					}
				}
				
				//Filter out a decimal number
				for (; i < line_len; i++)
				{
					if (line_char[i] >= '0' && line_char[i] <= '9')
					{
						continue;
					}
					else
					{
						break;
					}
				}

				for (; i < line_len; i++)
				{
					if (line_char[i] != ' ')
					{
						break;
					}
				}

				//Read symbol name
				symbol_name_len = 0;
				for (; i < line_len; i++)
				{
					if (line_char[i] != ' ' && line_char[i] != '\n' && line_char[i] != '\r' && line_char[i] != '\t' && line_char[i] != '\0')
					{
						symbol_name[symbol_name_len++] = line_char[i];
					}
					else
					{
						break;
					}
				}
				symbol_name[symbol_name_len] = 0x0;

				lines++;

				if (mips16_flag == 1)
				{
					symbol_addr += 1;
				}

				#if 0
				printf("%08x\n", symbol_addr);
				printf("symbol type = %s\n", symbol_type);
				printf("global type = %s\n", global_type);
				printf("symbol name = %s\n", symbol_name);
				printf("%s", line_char);
				#endif

				if (strcmp(symbol_type, "NOTYPE") == 0)
				{
					fprintf(out_apis, "\n#define att%s  0x%x\n\n", symbol_name, symbol_addr);
					continue;
				}
				else
				{
					if (mips16_flag == 1)
					{
						fprintf(out_apis, "%s = 0x%0x; // [%s][MIPS16]\n", symbol_name, symbol_addr, symbol_type);
					}
					else
					{
						fprintf(out_apis, "%s = 0x%0x; // [%s]\n", symbol_name, symbol_addr, symbol_type);
					}
				}
			}
		}
	}
	
	printf("gen export apis %d\n", lines);
	
	fclose(out_apis);
	fclose(in_symbols);
    
    return 0;
}
