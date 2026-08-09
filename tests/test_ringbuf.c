#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "uapi/vinput_uapi.h"
#include "vinput_ringbuf.h"

static int tests_passed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("[FAIL] %s\n", msg); \
            exit(1); \
        } else { \
            printf("[PASS] %s\n", msg); \
            tests_passed++; \
        } \
    } while (0)

int main(void) {
    struct vinput_ringbuf rb;
    struct vinput_event ev_in = {0}, ev_out = {0};
    int ret;

    printf("=== Starting Lockless Ring Buffer Unit Tests ===\n\n");

    /* Test 1: Initialization */
    vinput_ringbuf_init(&rb);
    TEST_ASSERT(vinput_ringbuf_is_empty(&rb) == true, "Buffer initialized empty");
    TEST_ASSERT(vinput_ringbuf_is_full(&rb) == false, "Buffer initialized not full");

    /* Test 2: Underflow Check */
    ret = vinput_ringbuf_pop(&rb, &ev_out);
    TEST_ASSERT(ret == -EAGAIN, "Pop on empty buffer returns -EAGAIN");

    /* Test 3: Single Push and Pop */
    ev_in.code = 30; /* KEY_A */
    ev_in.value = 1;  /* Press */
    ret = vinput_ringbuf_push(&rb, &ev_in);
    TEST_ASSERT(ret == 0, "Push single event succeeds");
    TEST_ASSERT(vinput_ringbuf_is_empty(&rb) == false, "Buffer is no longer empty");

    ret = vinput_ringbuf_pop(&rb, &ev_out);
    TEST_ASSERT(ret == 0, "Pop single event succeeds");
    TEST_ASSERT(ev_out.code == 30 && ev_out.value == 1, "Popped event matches pushed data");
    TEST_ASSERT(vinput_ringbuf_is_empty(&rb) == true, "Buffer is empty after pop");

    /* Test 4: Fill Buffer to Capacity */
    for (int i = 0; i < BUFFER_SIZE; i++) {
        ev_in.code = 100 + i;
        ret = vinput_ringbuf_push(&rb, &ev_in);
        TEST_ASSERT(ret == 0, "Push event to capacity");
    }

    TEST_ASSERT(vinput_ringbuf_is_full(&rb) == true, "Buffer reports FULL at capacity");

    /* Test 5: Overflow Protection */
    ev_in.code = 999;
    ret = vinput_ringbuf_push(&rb, &ev_in);
    TEST_ASSERT(ret == -ENOSPC, "Push on full buffer returns -ENOSPC");

    /* Test 6: Drain Buffer */
    for (int i = 0; i < BUFFER_SIZE; i++) {
        ret = vinput_ringbuf_pop(&rb, &ev_out);
        TEST_ASSERT(ret == 0 && ev_out.code == 100 + i, "Drain event maintains FIFO order");
    }

    TEST_ASSERT(vinput_ringbuf_is_empty(&rb) == true, "Buffer is empty after full drain");

    /* Test 7: Pointer Wrap-around Rollover */
    for (int cycle = 0; cycle < 1000; cycle++) {
        ev_in.code = cycle;
        vinput_ringbuf_push(&rb, &ev_in);
        vinput_ringbuf_pop(&rb, &ev_out);
        if (ev_out.code != cycle) {
            TEST_ASSERT(false, "Wrap-around rollover failed");
        }
    }
    TEST_ASSERT(true, "Wrap-around rollover (1000 iterations) succeeded");

    printf("\n=== All %d Tests Passed Successfully! ===\n", tests_passed);
    return 0;
}