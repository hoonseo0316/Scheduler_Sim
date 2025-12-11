#ifndef SIM_H
#define SIM_H

#include "request.h"

#define MAX_REQUEST 1024 // 흠.. Des
#define MAX_Batch 64 // 한 tick에 몇개를 태울 수 있는지 <- Hw 환경에 따라 정해짐

typedef struct SimState{
    int time; //현재 tick
    int max_time; //시뮬레이션을 얼마나 돌릴지

    int kv_budget; // kv budget, 이 이상이면 alive에 못 넣고 대기 -> latency 길어짐
    int kv_in_use; //현재 alive한 request들의 총 KV token합

    int max_batch;

} SimState; 

//=== util ===
// sim state initialize
void sim_init(
    SimState *state,
    int max_time,
    int kv_budget, //이거에 따라 동시에 올릴 수 있는 request 값이 달라짐... GPU memory에 올린다는 개념에서
    int max_batch // active request와는 별개
);

// 도착 시각이 된 Request들을 alive 상태로 전환
void sim_activate_arrivals(
    SimState *st,
    Request requests[],
    int num_requests); //number of requests 

// alive한 Request들의 인덱스를 모아오기
int sim_collect_alive_indices(
    Request requests[],
    int num_requests,
    int alive_indices[],
    int max_alives
);   // 살아있는 requests들)

// 선택된 request들에 대해 1 decode step 수행
void sim_run_decode_step(
    SimState *state,
    Request requests[],
    int selected_indices[],
    int selected_count
);

void sim_enforce_deadlines(
    SimState *state,
    Request requests[],
    int num_requests
);

// 모든 request가 끝났는지 여부 (1: 모두 완료, 0: 아직 남아 있음)
int sim_all_finished(
    Request requests[],
    int num_requests
);


#endif
