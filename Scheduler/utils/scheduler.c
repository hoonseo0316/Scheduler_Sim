#include "scheduler.h"

int scheduler_pick_batch_fifo(
    Request requests[], //전체 requests
    int alive_indices[], // alive 담는 buffer
    int alive_count, // sim.c에서 collect_alive_indices함수가 retrun하는 값
    SimState *state,
    int selected_indices[]   // 선택된 request 인덱스들 (requests 배열 기준)
){
    int limit = state -> max_batch;
    if(alive_count < limit){
        limit = alive_count;
    }

    // 앞에서부터 limit개 복사(limit보다 alive_count가 작으면 전부 갖고오고 아니면 limit만큼)
    for(int i = 0; i < limit; i++){
        selected_indices[i] = alive_indices[i];
    }
    return limit; //실제로 선택된 개수 외부에서 selected_indices[limit-1]까지를 descend
        

}