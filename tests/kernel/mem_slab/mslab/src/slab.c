/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test memory slab APIs
 *
 * This module tests the following memory slab routines:
 *
 *     k_mem_slab_alloc
 *     k_mem_slab_free
 *     k_mem_slab_num_used_get
 *
 * @note
 * One should ensure that the block is released to the same memory slab from which it
 * was allocated, and is only released once.  Using an invalid pointer will
 * have unpredictable side effects.
 */

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>

/* size of stack area used by each thread */
#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)

/* Number of memory blocks. The minimum number of blocks needed to run the
 * test is 2
 */
#define NUMBLOCKS   2

static int tc_rc = TC_PASS;     /* test case return code */

int test_slab_get_all_blocks(void **P);
int test_slab_free_all_blocks(void **P);


K_SEM_DEFINE(SEM_HELPERDONE, 0, 1);
K_SEM_DEFINE(SEM_REGRESSDONE, 0, 1);

K_MEM_SLAB_DEFINE(map_lgblks, 1024, NUMBLOCKS, 4);


/**
 *
 * @brief Verify return value
 *
 * This routine verifies current value against expected value
 * and returns true if they are the same.
 *
 * @param expect_ret_value     expect value
 * @param current_ret_current    current value
 *
 * @return  true, false
 */

bool verify_ret_value(int expect_ret_value, int current_ret_current)
{
	return (expect_ret_value == current_ret_current);

} /* verify_ret_value */

/**
 *
 * @brief Helper task
 *
 * This routine gets all blocks from the memory slab.  It uses semaphores
 * SEM_REGRESDONE and SEM_HELPERDONE to synchronize between different parts
 * of the test.
 *
 * @return  N/A
 */

void helper_thread(void)
{
	void *ptr[NUMBLOCKS];           /* Pointer to memory block */

	memset(ptr, 0, sizeof(ptr));    /* keep static checkers happy */
	/* Wait for part 1 to complete */
	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);

	/* Part 2 of test */

	TC_PRINT("Starts %s\n", __func__);
	PRINT_LINE;
	TC_PRINT("(2) - Allocate %d blocks in <%s>\n", NUMBLOCKS, __func__);
	PRINT_LINE;

	/* Test k_mem_slab_alloc */
	tc_rc = test_slab_get_all_blocks(ptr);
	if (tc_rc == TC_FAIL) {
		TC_ERROR("Failed test_slab_get_all_blocks function\n");
		goto exittest1;          /* terminate test */
	}

	k_sem_give(&SEM_HELPERDONE);  /* Indicate part 2 is complete */
	/* Wait for part 3 to complete */
	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);

	/*
	 * Part 4 of test.
	 * Free the first memory block.  RegressionTask is currently blocked
	 * waiting (with a timeout) for a memory block.  Freeing the memory
	 * block will unblock RegressionTask.
	 */
	PRINT_LINE;
	TC_PRINT("(4) - Free a block in <%s> to unblock the other task "
		 "from alloc timeout\n", __func__);
	PRINT_LINE;

	TC_PRINT("%s: About to free a memory block\n", __func__);
	k_mem_slab_free(&map_lgblks, &ptr[0]);
	k_sem_give(&SEM_HELPERDONE);

	/* Part 5 of test */
	k_sem_take(&SEM_REGRESSDONE, K_FOREVER);
	PRINT_LINE;
	TC_PRINT("(5) <%s> freeing the next block\n", __func__);
	PRINT_LINE;
	TC_PRINT("%s: About to free another memory block\n", __func__);
	k_mem_slab_free(&map_lgblks, &ptr[1]);

	/*
	 * Free all the other blocks.  The first 2 blocks are freed by this task
	 */
	for (int i = 2; i < NUMBLOCKS; i++) {
		k_mem_slab_free(&map_lgblks, &ptr[i]);
	}
	TC_PRINT("%s: freed all blocks allocated by this task\n", __func__);

exittest1:

	TC_END_RESULT(tc_rc);
	k_sem_give(&SEM_HELPERDONE);
}  /* helper thread */


/**
 *
 * @brief Get all blocks from the memory slab
 *
 * Get all blocks from the memory slab.  It also tries to get one more block
 * from the map after the map is empty to verify the error return code.
 *
 * This routine tests the following:
 *
 *   k_mem_slab_alloc(), k_mem_slab_num_used_get()
 *
 * @param p    pointer to pointer of allocated blocks
 *
 * @return  TC_PASS, TC_FAIL
 */

int test_slab_get_all_blocks(void **p)
{
	int ret_value;   /* task_mem_map_xxx interface return value */
	void *errptr;   /* Pointer to block */

	TC_PRINT("Function %s\n", __func__);

	for (int i = 0; i < NUMBLOCKS; i++) {
		/* Verify number of used blocks in the map */
		ret_value = k_mem_slab_num_used_get(&map_lgblks);
		if (verify_ret_value(i, ret_value)) {
			TC_PRINT("MAP_LgBlks used %d blocks\n", ret_value);
		} else {
			TC_ERROR("Failed task_mem_map_used_get for "
				 "MAP_LgBlks, i=%d, retValue=%d\n",
				 i, ret_value);
			return TC_FAIL;
		}

		/* Get memory block */
		ret_value = k_mem_slab_alloc(&map_lgblks, &p[i], K_NO_WAIT);
		if (verify_ret_value(0, ret_value)) {
			TC_PRINT("  k_mem_slab_alloc OK, p[%d] = %p\n",
				 i, p[i]);
		} else {
			TC_ERROR("Failed k_mem_slab_alloc, i=%d, "
				 "ret_value %d\n", i, ret_value);
			return TC_FAIL;
		}

	} /* for */

	/* Verify number of used blocks in the map - expect all blocks are
	 * used
	 */
	ret_value = k_mem_slab_num_used_get(&map_lgblks);
	if (verify_ret_value(NUMBLOCKS, ret_value)) {
		TC_PRINT("MAP_LgBlks used %d blocks\n", ret_value);
	} else {
		TC_ERROR("Failed task_mem_map_used_get for MAP_LgBlks, "
			 "retValue %d\n", ret_value);
		return TC_FAIL;
	}

	/* Try to get one more block and it should fail */
	ret_value = k_mem_slab_alloc(&map_lgblks, &errptr, K_NO_WAIT);
	if (verify_ret_value(-ENOMEM, ret_value)) {
		TC_PRINT("  k_mem_slab_alloc RC_FAIL expected as all (%d) "
			 "blocks are used.\n", NUMBLOCKS);
	} else {
		TC_ERROR("Failed k_mem_slab_alloc, expect RC_FAIL, got %d\n",
			 ret_value);
		return TC_FAIL;
	}

	PRINT_LINE;

	return TC_PASS;
}  /* test_slab_get_all_blocks */

/**
 *
 * @brief Free all memory blocks
 *
 * This routine frees all memory blocks and also verifies that the number of
 * blocks used are correct.
 *
 * This routine tests the following:
 *
 *   k_mem_slab_free(&), k_mem_slab_num_used_get(&)
 *
 * @param p    pointer to pointer of allocated blocks
 *
 * @return  TC_PASS, TC_FAIL
 */

int test_slab_free_all_blocks(void **p)
{
	int ret_value;     /* task_mem_map_xxx interface return value */

	TC_PRINT("Function %s\n", __func__);

	for (int i = 0; i < NUMBLOCKS; i++) {
		/* Verify number of used blocks in the map */
		ret_value = k_mem_slab_num_used_get(&map_lgblks);
		if (verify_ret_value(NUMBLOCKS - i, ret_value)) {
			TC_PRINT("MAP_LgBlks used %d blocks\n", ret_value);
		} else {
			TC_ERROR("Failed task_mem_map_used_get for "
				 "MAP_LgBlks, expect %d, got %d\n",
				 NUMBLOCKS - i, ret_value);
			return TC_FAIL;
		}

		TC_PRINT("  block ptr to free p[%d] = %p\n", i, p[i]);
		/* Free memory block */
		k_mem_slab_free(&map_lgblks, &p[i]);

		TC_PRINT("MAP_LgBlks freed %d block\n", i + 1);

	} /* for */

	/*
	 * Verify number of used blocks in the map
	 *  - should be 0 as no blocks are used
	 */

	ret_value = k_mem_slab_num_used_get(&map_lgblks);
	if (verify_ret_value(0, ret_value)) {
		TC_PRINT("MAP_LgBlks used %d blocks\n", ret_value);
	} else {
		TC_ERROR("Failed task_mem_map_used_get for MAP_LgBlks, "
			 "retValue %d\n", ret_value);
		return TC_FAIL;
	}

	PRINT_LINE;
	return TC_PASS;
}   /* testSlabFreeAllBlocks */

/**
 *
 * @brief Print the pointers
 *
 * This routine prints out the pointers.
 *
 * @param pointer    pointer to pointer of allocated blocks
 *
 * @return  N/A
 */
void print_pointers(void **pointer)
{
	TC_PRINT("%s: ", __func__);
	for (int i = 0; i < NUMBLOCKS; i++) {
		TC_PRINT("p[%d] = %p, ", i, pointer[i]);
	}

	TC_PRINT("\n");
	PRINT_LINE;

}  /* print_pointers */

/**
 *
 * @brief Main task to test memory slab interfaces
 *
 * This routine calls test_slab_get_all_blocks() to get all memory blocks from
 * the map and calls test_slab_free_all_blocks() to free all memory blocks.
 * It also tries to wait (with and without timeout) for a memory block.
 *
 * This routine tests the following:
 *
 *   k_mem_slab_alloc
 *
 * @return  N/A
 */

void main(void)
{
	int ret_value;             /* task_mem_map_xxx interface return value */
	void *b;                  /* Pointer to memory block */
	void *ptr[NUMBLOCKS];     /* Pointer to memory block */

	/* not strictly necessary, but keeps coverity checks happy */
	memset(ptr, 0, sizeof(ptr));

	/* Part 1 of test */

	TC_START("Test Kernel memory slabs");
	TC_PRINT("Starts %s\n", __func__);
	PRINT_LINE;
	TC_PRINT("(1) - Allocate and free %d blocks "
		 "in <%s>\n", NUMBLOCKS, __func__);
	PRINT_LINE;

	/* Test k_mem_slab_alloc */
	tc_rc = test_slab_get_all_blocks(ptr);
	if (tc_rc == TC_FAIL) {
		TC_ERROR("Failed test_slab_get_all_blocks function\n");
		goto exittest;           /* terminate test */
	}

	print_pointers(ptr);
	/* Test task_mem_map_free */
	tc_rc = test_slab_free_all_blocks(ptr);
	if (tc_rc == TC_FAIL) {
		TC_ERROR("Failed testalab_freeall_blocks function\n");
		goto exittest;           /* terminate test */
	}

	k_sem_give(&SEM_REGRESSDONE);   /* Allow helper thread to run */
	/* Wait for helper thread to finish */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);

	/*
	 * Part 3 of test.
	 *
	 * helper thread got all memory blocks.  There is no free block left.
	 * The call will timeout.  Note that control does not switch back to
	 * helper thread as it is waiting for SEM_REGRESSDONE.
	 */

	PRINT_LINE;
	TC_PRINT("(3) - Further allocation results in  timeout "
		 "in <%s>\n", __func__);
	PRINT_LINE;

	ret_value = k_mem_slab_alloc(&map_lgblks, &b, 20);
	if (verify_ret_value(-EAGAIN, ret_value)) {
		TC_PRINT("%s: k_mem_slab_alloc times out which is "
			 "expected\n", __func__);
	} else {
		TC_ERROR("Failed k_mem_slab_alloc, retValue %d\n", ret_value);
		tc_rc = TC_FAIL;
		goto exittest;           /* terminate test */
	}

	TC_PRINT("%s: start to wait for block\n", __func__);
	k_sem_give(&SEM_REGRESSDONE);    /* Allow helper thread to run part 4 */
	ret_value = k_mem_slab_alloc(&map_lgblks, &b, 50);
	if (verify_ret_value(0, ret_value)) {
		TC_PRINT("%s: k_mem_slab_alloc OK, block allocated at %p\n",
			 __func__, b);
	} else {
		TC_ERROR("Failed k_mem_slab_alloc, ret_value %d\n", ret_value);
		tc_rc = TC_FAIL;
		goto exittest;           /* terminate test */
	}

	/* Wait for helper thread to complete */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);

	TC_PRINT("%s: start to wait for block\n", __func__);
	k_sem_give(&SEM_REGRESSDONE);    /* Allow helper thread to run part 5 */
	ret_value = k_mem_slab_alloc(&map_lgblks, &b, K_FOREVER);
	if (verify_ret_value(0, ret_value)) {
		TC_PRINT("%s: k_mem_slab_alloc OK, block allocated at %p\n",
			 __func__, b);
	} else {
		TC_ERROR("Failed k_mem_slab_alloc, ret_value %d\n", ret_value);
		tc_rc = TC_FAIL;
		goto exittest;           /* terminate test */
	}

	/* Wait for helper thread to complete */
	k_sem_take(&SEM_HELPERDONE, K_FOREVER);


	/* Free memory block */
	TC_PRINT("%s: Used %d block\n", __func__,
		 k_mem_slab_num_used_get(&map_lgblks));
	k_mem_slab_free(&map_lgblks, &b);
	TC_PRINT("%s: 1 block freed, used %d block\n",
		 __func__,  k_mem_slab_num_used_get(&map_lgblks));

exittest:

	TC_END_RESULT(tc_rc);
	TC_END_REPORT(tc_rc);
}

K_THREAD_DEFINE(HELPER, STACKSIZE, helper_thread, NULL, NULL, NULL,
		7, 0, K_NO_WAIT);
