#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
	FILE *in_make_cfg, *in_export_apis, *out_make_cfg_out;
	char *make_cfg_name;
	char *export_apis_name;
	char *make_cfg_out_name;
	unsigned char line_chars[1024];
	unsigned char line_chars_apis[1024];
	unsigned char line_chars_out[1024];
	int line_len, line_len_apis, line_len_out, i, j;
	unsigned char *p_load_addr;
	unsigned char *p_run_addr;
	int load_addr_len;
	int run_addr_len;
	unsigned char *p_api_name_addr;
	int api_name_len;
	unsigned char *p_api_value_addr;
	int api_value_len;

	if (argc != 4)
	{
		printf("argc != 4\n");
		return 0;
	}
	
	make_cfg_name = argv[1];
	export_apis_name = argv[2];
	make_cfg_out_name = argv[3];
	
	in_make_cfg = fopen(make_cfg_name, "rb");
	if (in_make_cfg == NULL)
	{
		printf("Not find [%s]!\n", make_cfg_name);
		return 0;
	}

	in_export_apis = fopen(export_apis_name, "rb");
	if (in_export_apis == NULL)
	{
		printf("Not find [%s]!\n", export_apis_name);
		return 0;
	}

	out_make_cfg_out = fopen(make_cfg_out_name, "wb");
	if (out_make_cfg_out == NULL)
	{
		printf("Create [%s] Fail!\n", make_cfg_out_name);
		fclose(out_make_cfg_out);
		return 0;
	}
	
	while (1)
	{
		if (feof(in_make_cfg))
		{
			break;
		}
		
		memset(line_chars, 0, 1024);
		
		if (fgets(line_chars, 1023, in_make_cfg) == NULL)
		{
			break;
		}
		
		line_len = strlen(line_chars);
		
		if (line_len < strlen("ATT_PATTERN_FILE"))
		{
			fputs(line_chars, out_make_cfg_out);
			continue;
		}
		
		line_len_out = 0;
		i = 0;
		//delete space char
		for (; i < line_len; i++)
		{
			if (line_chars[i] != ' ')
			{
				break;
			}
		}
		
		//if is ATT_PATTERN_FILE?
		if (strncmp(line_chars + i, "ATT_PATTERN_FILE", strlen("ATT_PATTERN_FILE")) != 0)
		{
			fputs(line_chars, out_make_cfg_out);
			continue;
		}
		
		//jump over keyword ATT_PATTERN_FILE
		i += strlen("ATT_PATTERN_FILE");
		
		//delete space char
		for (; i < line_len; i++)
		{
			if (line_chars[i] != ' ')
			{
				break;
			}
		}
		
		if (line_chars[i] != '=')
		{
			fputs(line_chars, out_make_cfg_out);
			printf("ATT_PATTERN_FILE format invalid, =\n");
			continue;
		}
		
		//jump over =
		i += 1;

		//delete space char
		for (; i < line_len; i++)
		{
			if (line_chars[i] != ' ')
			{
				break;
			}
		}
		
		//filter out name, " " or "," endl
		for (; i < line_len; i++)
		{
			if (line_chars[i] == ' ' || line_chars[i] == ',')
			{
				break;
			}
		}
		
		//delete space char
		for (; i < line_len; i++)
		{
			if (line_chars[i] != ' ')
			{
				break;
			}
		}
		
		if (line_chars[i] != ',')
		{
			fputs(line_chars, out_make_cfg_out);
			printf("ATT_PATTERN_FILE format invalid, , 1st\n");
			continue;
		}
		
		//jump over ,
		i += 1;
		
		//delete space char
		for (; i < line_len; i++)
		{
			if (line_chars[i] != ' ')
			{
				break;
			}
		}
		
		memset(line_chars_out, 0x0, 1024);
		line_len_out = i;
		memcpy(line_chars_out, line_chars, line_len_out);
		
		//filter out name, " " or "," endl
		p_load_addr = line_chars + i;
		load_addr_len = 0;
		for (; i < line_len; i++)
		{
			if (line_chars[i] == ' ' || line_chars[i] == ',')
			{
				break;
			}
			else
			{
				load_addr_len++;
			}
		}
		
		if (strncmp(p_load_addr, "0x", 2) != 0)
		{
			fseek(in_export_apis, 0, 0);
		
			while (1)
			{
				if (feof(in_export_apis))
				{
					break;
				}
				
				memset(line_chars_apis, 0, 1024);
				
				if (fgets(line_chars_apis, 1023, in_export_apis) == NULL)
				{
					break;
				}
				
				line_len_apis = strlen(line_chars_apis);
				
				if (strncmp(line_chars_apis, "#define", strlen("#define")) == 0)
				{
					j = strlen("#define");
					
					//filter space char
					for (; j < line_len_apis; j++)
					{
						if (line_chars_apis[j] != ' ')
						{
							break;
						}
					}
					
					//read symble name, as space char the endl
					p_api_name_addr = line_chars_apis + j;
					api_name_len = 0;
					for (; j < line_len_apis; j++)
					{
						if (line_chars_apis[j] == ' ')
						{
							break;
						}
						else
						{
							api_name_len++;
						}
					}
					
					//filter out space char
					for (; j < line_len_apis; j++)
					{
						if (line_chars_apis[j] != ' ')
						{
							break;
						}
					}
					
					//read address, space char, line feeds char, endl
					p_api_value_addr = line_chars_apis + j;
					api_value_len = 0;
					for (; j < line_len_apis; j++)
					{
						if (line_chars_apis[j] == ' ' || line_chars_apis[j] == 0x0 || line_chars_apis[j] == '\n' || line_chars_apis[j] == '\r')
						{
							break;
						}
						else
						{
							api_value_len++;
						}
					}
					
					//Compare names
					if ((load_addr_len == api_name_len) && (strncmp(p_load_addr, p_api_name_addr, load_addr_len) == 0))
					{
						//Replacement address
						memcpy(line_chars_out + line_len_out, p_api_value_addr, api_value_len);
						line_len_out += api_value_len;
						break;
					}
				}
			}
		}
		else
		{
			memcpy(line_chars_out + line_len_out, p_load_addr, load_addr_len);
			line_len_out += load_addr_len;
		}

		//delete space char
		for (; i < line_len; i++)
		{
			if (line_chars[i] != ' ')
			{
				break;
			}
			else
			{
				line_chars_out[line_len_out++] = line_chars[i];
			}
		}
		
		if (line_chars[i] != ',')
		{
			fputs(line_chars, out_make_cfg_out);
			printf("ATT_PATTERN_FILE format invalid, , 2nd\n");
			continue;
		}
		else
		{
			line_chars_out[line_len_out++] = line_chars[i];
		}

		//jump over ,
		i += 1;

		//delete space char
		for (; i < line_len; i++)
		{
			if (line_chars[i] != ' ')
			{
				break;
			}
			else
			{
				line_chars_out[line_len_out++] = line_chars[i];
			}
		}

		//filter out name, " " or "\n" or "\r" end
		p_run_addr = line_chars + i;
		run_addr_len = 0;
		for (; i < line_len; i++)
		{
			if (line_chars[i] == ' ' || line_chars[i] == 0x0 || line_chars[i] == '\n' || line_chars[i] == '\r')
			{
				break;
			}
			else
			{
				run_addr_len++;
			}
		}
		
		if (strncmp(p_run_addr, "0x", 2) != 0)
		{
			fseek(in_export_apis, 0, 0);
		
			while (1)
			{
				if (feof(in_export_apis))
				{
					break;
				}
				
				memset(line_chars_apis, 0, 1024);
				
				if (fgets(line_chars_apis, 1023, in_export_apis) == NULL)
				{
					break;
				}
				
				line_len_apis = strlen(line_chars_apis);
				
				if (strncmp(line_chars_apis, "#define", strlen("#define")) == 0)
				{
					j = strlen("#define");
					
					//filter out space char
					for (; j < line_len_apis; j++)
					{
						if (line_chars_apis[j] != ' ')
						{
							break;
						}
					}
					
					//read symbol name, space char end
					p_api_name_addr = line_chars_apis + j;
					api_name_len = 0;
					for (; j < line_len_apis; j++)
					{
						if (line_chars_apis[j] == ' ')
						{
							break;
						}
						else
						{
							api_name_len++;
						}
					}
					
					//filter out space char
					for (; j < line_len_apis; j++)
					{
						if (line_chars_apis[j] != ' ')
						{
							break;
						}
					}
					
					//read address, space char, line feeds char, endl
					p_api_value_addr = line_chars_apis + j;
					api_value_len = 0;
					for (; j < line_len_apis; j++)
					{
						if (line_chars_apis[j] == ' ' || line_chars_apis[j] == 0x0 || line_chars_apis[j] == '\n' || line_chars_apis[j] == '\r')
						{
							break;
						}
						else
						{
							api_value_len++;
						}
					}
					
					//Compare names
					if ((run_addr_len == api_name_len) && (strncmp(p_run_addr, p_api_name_addr, run_addr_len) == 0))
					{
						//Replacement address
						memcpy(line_chars_out + line_len_out, p_api_value_addr, api_value_len);
						line_len_out += api_value_len;
						break;
					}
				}
			}
		}
		else
		{
			memcpy(line_chars_out + line_len_out, p_run_addr, run_addr_len);
			line_len_out += run_addr_len;
		}

		for (; i < line_len; i++)
		{
			line_chars_out[line_len_out++] = line_chars[i];
			if (line_chars[i] == 0x0)
			{
				break;
			}
		}
		
		fputs(line_chars_out, out_make_cfg_out);
	}

	printf("gen %s file successfully!\n", make_cfg_out_name);
	
	fclose(in_make_cfg);
	fclose(in_export_apis);
	fclose(out_make_cfg_out);
    
    return 0;
}
