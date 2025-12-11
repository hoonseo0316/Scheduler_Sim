#include "request.h"

 void request_init(
    Request *req,
    int id,
    int arrival_time,
    int total_tokens,
    int kv_tokens,
    int deadline
) {
    req -> id = id;
    req -> arrival_time     = arrival_time;

    req -> total_tokens     = total_tokens;
    req -> remaining_tokens = total_tokens; //initialize 단계에서 total token 배치
    req -> kv_tokens        = kv_tokens;
    
    req -> deadline         = deadline;
    req -> finished_time    = NOT_FINISHED; //헤더 파일에서 -1로 선언해두었다. 안 끝났음

    req -> alive_flag       = 0; //도착 순간부터 1; 도착하기 전과 sequence 끝나면 0;
}

int request_is_finished(const Request *req){
   return (req -> finished_time != NOT_FINISHED);
}

int request_is_alive(const Request *req){
    return req -> alive_flag;
}

int request_latency(const Request *req){
    if(!request_is_finished(req)){
        return -1; //latency가 없음을 의미
    }
    else return ((req -> finished_time) - (req -> arrival_time));
}