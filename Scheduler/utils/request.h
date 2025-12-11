#ifndef REQUEST_H
#define REQUEST_H

#ifdef __cplusplus
extern "C" {
#endif

// 특수 값 정의
#define NO_DEADLINE  (-1) //deadline이 없을 때
#define NOT_FINISHED (-1) //아직 request가 안 끝났을 때

// LLM DECODE에서 "하나의 시퀀스 / 요청"을 나타내는 구조체
typedef struct Request {
    int id; // 내부 식별자 번호
    int arrival_time; // request 된 시각(tick)

    int total_tokens; // 이 sequence의 길이 = 필요한 decode step 수
    int remaining_tokens; // 남은 steps

    int kv_tokens; //이 job의 KV cache

    int deadline; //이 request가 완료되어야 하는 시간: tick
    int finished_time; //이 request가 끝난 시각

    int alive_flag; // 살아있는지의 여부
} Request;

// === Utils for Request ===

void request_init(
    Request *req,
    int id,
    int arrival_time,
    int total_tokens,
    int kv_tokens,
    int deadline
);

// 이 요청이 완료되었는지 여부, const로 내용 fix
int request_is_finished(const Request *req);

//alive 플래그 편하게 체크하는 헬퍼
// req->alive로 접근해도 되지만,,,
int request_is_alive(const Request *req);
// latency 계산
int request_latency(const Request *req);

#ifdef __cplusplus
}

#endif

#endif // REQUEST_H
