#ifndef _KERNEL_VERSION_H_
#define _KERNEL_VERSION_H_

#define ZEPHYR_VERSION_CODE 67840
#define ZEPHYR_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#define KERNELVERSION \
0x01090000
#define KERNEL_VERSION_NUMBER     0x010900
#define KERNEL_VERSION_MAJOR      1
#define KERNEL_VERSION_MINOR      9
#define KERNEL_PATCHLEVEL         0
#define KERNEL_VERSION_STRING     "1.9.0"
#define KERNEL_BUILD_DATE         "220418"
#define KERNEL_BUILD_TIME         "202043"

#endif /* _KERNEL_VERSION_H_ */
