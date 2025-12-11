#include <stdio.h>
#include "utils\sim.h"
#include "utils\scheduler.h"
#include "utils\request.h"

// ---- 시뮬레이션 설정 상수 ----
#define NUM_REQUESTS 3
#define MAX_TIME     100
#define KV_BUDGET    16
#define MAX_BATCH    2

int main(void)
{
    // 1) 상태/요청 배열 준비
    SimState state;
    Request  requests[NUM_REQUESTS];

    // 2) Request 초기화
    //    id, arrival_time, total_tokens, kv_tokens, deadline
    //    deadline에 NO_DEADLINE 넣으면 데드라인 없는 요청
    request_init(&requests[0], 0, /*arrival*/ 0, /*tokens*/ 5, /*kv*/ 4,  20);
    request_init(&requests[1], 1, /*arrival*/ 2, /*tokens*/ 8, /*kv*/ 8, NO_DEADLINE);
    request_init(&requests[2], 2, /*arrival*/ 4, /*tokens*/ 6, /*kv*/ 4,  30);

    // 3) 시뮬레이터 초기화
    sim_init(&state, MAX_TIME, KV_BUDGET, MAX_BATCH);

    int alive_indices[NUM_REQUESTS];
    int selected_indices[NUM_REQUESTS];

    printf("=== KV-aware decode scheduler simulation start ===\n");

    // 4) 메인 시뮬레이션 루프
    while (state.time < state.max_time &&
           !sim_all_finished(requests, NUM_REQUESTS)) {

        printf("\n[time = %d]\n", state.time);

        // (1) deadline 위반 처리 (타임아웃 request 정리)
        sim_enforce_deadlines(&state, requests, NUM_REQUESTS);

        // (2) 새로 도착한 request들을 alive로 올리기 (kv_budget 고려)
        sim_activate_arrivals(&state, requests, NUM_REQUESTS);

        // (3) 현재 alive 집합 수집
        int alive_count = sim_collect_alive_indices(
            requests,
            NUM_REQUESTS,
            alive_indices,
            NUM_REQUESTS  // 지금은 max_alive = 전체 개수로 사용
        );

        printf("  alive_count = %d, kv_in_use = %d / %d\n",
               alive_count, state.kv_in_use, state.kv_budget);

        // alive가 하나도 없으면 시간만 흘려보냄
        if (alive_count == 0) {
            printf("  no alive requests this tick.\n");
            state.time++;
            continue;
        }

        // (4) 스케줄러가 이번 step에 돌릴 배치를 선택 (FIFO)
        int selected_count = scheduler_pick_batch_fifo(
            requests,
            alive_indices,
            alive_count,
            &state,
            selected_indices
        );

        printf("  selected_count = %d, selected_indices = { ", selected_count);
        for (int i = 0; i < selected_count; i++) {
            printf("%d ", selected_indices[i]);
        }
        printf("}\n");

        // (5) 선택된 request들에 대해 decode 1 step 수행
        sim_run_decode_step(
            &state,
            requests,
            selected_indices,
            selected_count
        );

        // (6) 다음 time step으로
        state.time++;
    }

    printf("\n=== simulation finished at time %d ===\n", state.time);

    // 5) 최종 결과 출력
    for (int i = 0; i < NUM_REQUESTS; i++) {
        const Request *req = &requests[i];
        int latency = request_latency(req);

        printf("Request %d:\n", req->id);
        printf("  arrival_time     = %d\n", req->arrival_time);
        printf("  total_tokens     = %d\n", req->total_tokens);
        printf("  kv_tokens        = %d\n", req->kv_tokens);
        printf("  deadline         = %d\n", req->deadline);
        printf("  finished_time    = %d\n", req->finished_time);
        printf("  remaining_tokens = %d\n", req->remaining_tokens);
        printf("  alive_flag       = %d\n", req->alive_flag);
        printf("  latency          = %d\n", latency); // -1이면 아직 안 끝난 것
        printf("\n");
    }

    return 0;
}
