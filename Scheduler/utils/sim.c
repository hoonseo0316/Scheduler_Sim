#include "sim.h"

void sim_init(
    SimState *state,
    int max_time,
    int kv_budget, //이거에 따라 동시에 올릴 수 있는 request 값이 달라짐... GPU memory에 올린다는 개념에서
    int max_batch // active request와는 별개
){
    state -> time      = 0;
    state -> max_time  = max_time;

    state -> kv_budget = kv_budget;
    state -> kv_in_use = 0;
 
    state -> max_batch = max_batch;
}

// request를 모두 생성해서 list에 넣어두고, 뱅글뱅글 돌면서 검증하는 방식
// 항상 소거법형태, alive인지 끝났는지 배재하는 방식으로 alive를 관리


// 도착 시각이 된 Request들을 alive 상태로 전환
// 이미 on된 애들 배제 -> 죽은 애들 배제 -> 아직 활성화되지 않을 애들 배제
// kv cache budget 내부면 올리기, 아니면 꺼두기 (공간 나면 어차피 올라감)

void sim_activate_arrivals(
    SimState *state,
    Request requests[],
    int num_requests //총 리퀘스트 양 <- 초기 값(이건 dynamic하게 하기 보단, 전체 양을 정하고 시간 지나면 키는 형태로 요청 들어오는 걸 구현)
){
    for(int i= 0; i<num_requests; i++){
        Request *req = &requests[i];
        
        // 이미 올라간 request는 skip
        if(request_is_alive(req)){
            continue;
        }

        // finished된 request도 skip
        if(request_is_finished(req)){
            continue;
        }
        
        //아직 arrival time이 안 된 애들
        if(req -> arrival_time > state -> time){
            continue;
        }
        // arrival time>=time인 상황 + 끝나지 않고, 이미 활성화되지 않은 애들
        if(req->kv_tokens + state -> kv_in_use <= state -> kv_budget){
            req-> alive_flag = 1;
            state -> kv_in_use += req -> kv_tokens;
        }
    }
    
}

// alive한 Request들의 인덱스를 모아오기
// 전체 loop 돌면서, 비활성화된 애들 제외, 끝난 애들 제외하고, 남은 애들의 index를 저장
// alive일 때만 count가 오르며, i는 계속 오름 <- 활성화된 애들의 주소만 저장해서 alive_indices를 덮음
// count까지 현재 살아있는 애들
int sim_collect_alive_indices(
    Request requests[],
    int num_requests, 
    int alive_indices[],   // 살아있는 requests들
    int max_alive //나중에 확장할 때 사용; 우선은 request 개수는 신경 쓰지 않기로.
    //어차피 budget 못맞추면 alive에 못올라가서 ㄱㅊ 
){
    int  count = 0;
    for(int i=0; i<num_requests; i++){

        Request *req = &requests[i];
        if(!request_is_alive(req)){
            continue;
        }

        if(request_is_finished(req)){
            continue;
        }

        //input space 상으로 여집합 소거 한건데, 좀비 상태(alive_flag=1, finished_t != Not_finished)
        //여도 제거할 수 있도록...
        alive_indices[count] = i;
        count++;
    }
    return count; //alive 주소 alive_indices[count-1]까지가 살아있는 애들 주소

    }


// 선택된 request들에 대해 1 decode step 수행
void sim_run_decode_step(
    SimState *state,
    Request requests[],
    int selected_indices[],
    int selected_count
){
    
    for(int i=0;i<selected_count;i++){ //선택된 개수만큼 loop으로 scan하면서
        int selected_indx = selected_indices[i];
        Request *req      = &requests[selected_indx];

        if(req -> remaining_tokens > 0){ // 종료조건이 아닐시 차감
            req->remaining_tokens--;

            if(req -> remaining_tokens == 0 && req->finished_time == NOT_FINISHED){
                //0이 됐으니까, 끝난 상태로 변경   ^^^^ 이 조건 없으면 우연히 들어온 좀비의 상태가 변경..
                req -> finished_time = state -> time;
                req -> alive_flag    = 0;
                state -> kv_in_use -= req->kv_tokens; //캐시리턴
                if(state -> kv_in_use < 0){
                    state ->kv_in_use =0;
                }
            }

            
        }
    }

}

void sim_enforce_deadlines(
    SimState *state,
    Request requests[],
    int num_requests
){
    for (int i = 0; i < num_requests; i++) {
        Request *req = &requests[i];

        // deadline이 없는 경우는 스킵 (NO_DEADLINE 같은 값이면)
        if (req->deadline == NO_DEADLINE) {
            continue;
        }

        // 이미 종료 처리된 애는 스킵
        if (req->finished_time != NOT_FINISHED) {
            continue;
        }

        // deadline을 넘겼다면: 더 이상 돌리지 않고 "타임아웃 종료" 처리
        if (state->time > req->deadline && req->finished_time == NOT_FINISHED) {
            req->finished_time = state->time; // "실패 시각"

            if (req->alive_flag) {
                req->alive_flag = 0;
                state->kv_in_use -= req->kv_tokens;
                if (state->kv_in_use < 0) {
                    state->kv_in_use = 0;
                }
            }

            // remaining_tokens는 > 0인 채로 남겨둠
            // → "deadline miss"라는 정보가 남음
        }
    }
}

// 모든 request가 끝났는지 여부 (1: 모두 완료, 0: 아직 남아 있음)
int sim_all_finished(
    Request requests[],
    int num_requests
){
    for (int i = 0; i < num_requests; i++) {
        if (!request_is_finished(&requests[i])) {
            return 0;  // 하나라도 안 끝났으면 아직 진행 중
        }
    }
    return 1;  // 전부 finished
}

