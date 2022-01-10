#ifndef _RES_FINE_TUNE_H_
#define _RES_FINE_TUNE_H_

typedef struct {
	int channel;//0-left channel, 1-right channel
	int mode;  //		mode=[				-3,				-2,				-1,				0,				1,					2,				3]
	         	//								�µ�5��		�µ�3��		�µ�1��	  ����			�ϵ�1��			�ϵ�3��		�ϵ�5�� 
	int insample_rate;// ��������������  
	int outsample_rate;// ������������ʡ� 
	int in_samp;	//�������������Ҫ��С��256������.
	int out_samp;
	short *input_buf;
	short *output_buf;
	unsigned char *shared_buf;
	int shared_size;

}as_res_finetune_params;

typedef enum
{
	RES_FINETUNE_CMD_OPEN = 0,
	RES_FINETUNE_CMD_CLOSE,
	RES_FINETUNE_CMD_PROCESS,
	RES_FINETUNE_CMD_MEMORY,

} as_res_finetune_ops_cmd_t;

int as_res_finetune_ops(void *hnd, as_res_finetune_ops_cmd_t cmd, unsigned int args);

#endif

/*
void main()
{
int len;
int file_len;
unsigned char *inbuf, *outbuf;
unsigned char *inbuf_ptr, *outbuf_ptr;
as_res_finetune_params param;
int handle = 0;
int rt;


memset(&param, 0, sizeof(as_res_finetune_params));
param.mode = 0;
param.insample_rate = 44100;//
param.outsample_rate = 48000;//	

����1������RES_FINETUNE_CMD_MEMORY�� ϵͳ��param.shared_size�����ֽڣ���global�ռ俪�ڴ�
rt = as_res_finetune_ops(handle, RES_FINETUNE_CMD_MEMORY, &param);
param.shared_buf = (unsigned char *)malloc(param.shared_size);
memset(param.shared_buf, 0, param.shared_size);
inbuf = malloc( param.in_samp*sizeof(short));//�����������BUFF�����Ա���С�������ܳ�����
outbuf = malloc(param.out_samp*sizeof(short))//����������BUFF


//����2��RES_FINETUNE_CMD_OPEN�� 	
rt = as_res_finetune_ops(&handle, RES_FINETUNE_CMD_OPEN, &param);



while (1)
{
param.input_buf = inbuf;
param.output_buf = outbuf;


//����3���ز���΢��
rt = as_res_finetune_ops(handle, RES_FINETUNE_CMD_PROCESS, &param);
fwrite(outbuf, param.out_samp*sizeof(short), 1, fin);
inbuf += param.in_samp*sizeof(short);

if (.....)
{//����ʵ�ʰ�������״̬���ʵ��޸�param.mode

param.mode = 1;// -3,-2,-1,0,1,2,3
}
}

//����4������
as_res_finetune_ops(handle, RES_FINETUNE_CMD_CLOSE, 0);
return;
}*/

