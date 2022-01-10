/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_cdata.h"

/**
 * @addtogroup t_threads_customdata
 * @{
 * @defgroup t_threads_customdata_api test_threads_customdata_api
 * @}
 */

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_customdata_api,
		ztest_unit_test(test_customdata_get_set_coop),
		ztest_unit_test(test_customdata_get_set_preempt));
	ztest_run_test_suite(test_customdata_api);
}
