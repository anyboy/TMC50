#ifndef _RES_FINE_TUNE_H_
#define _RES_FINE_TUNE_H_

typedef struct {
	int channel;//0-left channel, 1-right channel
	int mode;  //		mode=[				-3,				-2,				-1,				0,				1,					2,				3]
	         	//								下调5‰		下调3‰		下调1‰	  不调			上调1‰			上调3‰		上调5‰ 
	int insample_rate;// 输入码流采样率  
	int outsample_rate;// 输出码流采样率。 
	int in_samp;	//输入样点个数，要求小于256个样点.
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

步骤1：调用RES_FINETUNE_CMD_MEMORY， 系统按param.shared_size返回字节，在global空间开内存
rt = as_res_finetune_ops(handle, RES_FINETUNE_CMD_MEMORY, &param);
param.shared_buf = (unsigned char *)malloc(param.shared_size);
memset(param.shared_buf, 0, param.shared_size);
inbuf = malloc( param.in_samp*sizeof(short));//输入码流最大BUFF，可以比它小，但不能超过它
outbuf = malloc(param.out_samp*sizeof(short))//输出码流最大BUFF


//步骤2：RES_FINETUNE_CMD_OPEN， 	
rt = as_res_finetune_ops(&handle, RES_FINETUNE_CMD_OPEN, &param);



while (1)
{
param.input_buf = inbuf;
param.output_buf = outbuf;


//步骤3：重采样微调
rt = as_res_finetune_ops(handle, RES_FINETUNE_CMD_PROCESS, &param);
fwrite(outbuf, param.out_samp*sizeof(short), 1, fin);
inbuf += param.in_samp*sizeof(short);

if (.....)
{//根据实际半满或半空状态，适当修改param.mode

param.mode = 1;// -3,-2,-1,0,1,2,3
}
}

//步骤4：结束
as_res_finetune_ops(handle, RES_FINETUNE_CMD_CLOSE, 0);
return;
}*/

