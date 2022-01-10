#ifndef _ATT_ENV_VAR_H_
#define _ATT_ENV_VAR_H_

#include "att_stub_cmd_def.h"
#include "att_testid_def.h"
#include "att_protocol_def.h"

struct att_code_info {
    unsigned char testid;
    unsigned char type;
    unsigned char need_drv;
    const char    pt_name[13]; //8+1+3+'\0'
};

struct att_drv_code_info {
    const char    pt_name[13]; //8+1+3+'\0'
    const char    drv_name[13]; //8+1+3+'\0'
};

struct att_env_var {
	u8_t  *rw_temp_buffer;
	u8_t  *return_data_buffer;
	u16_t test_id;
	u16_t arg_len;
	u16_t test_mode;	//see test_mode_e
	u16_t exit_mode;	//see att_exit_mode_e
};

#endif /* _ATT_ENV_VAR_H_ */
