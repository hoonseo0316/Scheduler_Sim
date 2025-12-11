#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "sim.h"
#include "request.h"

// FIFO 스케줄러: alive_indices 순서대로 최대 state->max_batch까지 선택
int scheduler_pick_batch_fifo(
    Request requests[], //전체 requests
    int alive_indices[], // alive 담는 buffer
    int alive_count, // sim.c에서 collect_alive_indices함수가 retrun하는 값
    SimState *state,
    int selected_indices[]   // 선택된 request 인덱스들 (requests 배열 기준)
);

#endif