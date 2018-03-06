
#ifndef RAFT_PROJECT_GENERAL_H
#define RAFT_PROJECT_GENERAL_H


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <bits/signum.h>
#include <signal.h>
#include <sys/time.h>

#include "logger.h"
//IMPORTANT!!! should be 1 if we are in DEBOG_MODE
///////////////////////////////////////////
#define DEBUG_MODE 1
//////////////////////////////////////////
//#define LEADER
//#define CANDIDATE
//#define FOLLOWER
//states

//log levels
#define DEBUG_LEVEL 0
#define INFO_LEVEL  1
#define WARN_LEVEL  2
#define ERROR_LEVEL 3
#define FATAL_LEVEL 4

#define CURRENT_STATE_KEY "currentState"
#define LEADER_STATE_VALUE "leader"

#if DEBUG_MODE == 1
	#define MAX_RAFT_TIMEOUT 3000
	#define MIN_RAFT_TIMEOUT 1000
#else
	#define MAX_RAFT_TIMEOUT 300
	#define MIN_RAFT_TIMEOUT 150
#endif
#define QUEUE_SIZE       555
#define MAX_ELEM_SIZE     51

#define PYTHON_DELIMITER ","

#define MILISEC_CONVERT 1000


//db types
#define DB_LOG "log db"
#define DB_VAL "valuse db"
#define DB_CONFIG "config db"
#define DB_STATUS "status db"

//redis STATUS  DB fields
#define STATUS "status"
#define LEADER_ID "leader id"
#define LAST_APPLIED "last applied"
#define COMMIT_INDEX "commit index"
#define TERM "term"


//redis CONFIGURATION  DB fields
#define MEMBERS_AMOUNT "members amount"
#define MULTICAST_IP "multicast ip"
#define CLUSTER_ID "cluster id"
#define LEADER_TIME_OUT "leader time out"
#define DEBBUG_MODE "debbug mode"
#define REDIS_IP "redis ip"
#define REDIS_PORT "redis port"



#define STATE_AMMOUNT 3
#define EVENT_AMMOUNT 10

#define WRITE_TO_LOGGER(logger_level,problem_desc,msg_type,args_num,...) ( \
				__WRITE_TO_LOGGER__(sharedRaftData.python_functions.write_to_logger,logger_level,problem_desc,msg_type,args_num,##__VA_ARGS__) )

typedef enum EventType{TIMEOUT=0, KEEP_ALIVE_HB, SET_LOG_HB, SET_LOG_RES, SYNC_REQ, SYNC_RES, COMMIT_OK, VOTE, REQUEST_FOR_VOTE,PY_SEND_LOG}eventType;
typedef enum StateType{FOLLOWER=0,CANDIDATE,LEADER}stateType;

typedef struct State{
    int term;
    int last_log_index;
    int last_commit_index;
    char current_state; //F/C/L //TBD- why not enum?????
    int members_amount;
    int server_id;

    //for candidate use
    int vote_counter;

    //for leader use
    int commit_counter;

    //for follower use
    int did_I_vote;

    int timeout;

    //if wakeup_counter == 2 , follower -> candidate
    int wakeup_counter;

    int leader_id;

    //all 10 options
    eventType event_type;
    //not nedded for now
    //State (*hendler)();
}state;


typedef struct Keep_alive_hb{
    int last_log_id;
}keep_alive_hb;

typedef struct Set_log_hb{
    int commit_id;
    char cmd[MAX_ELEM_SIZE];
    char key[MAX_ELEM_SIZE];
    char value[MAX_ELEM_SIZE];

}set_log_hb;

typedef struct Sync_req{
    int start_log_index;
    int last_log_id;
}sync_req;

typedef struct Sync_res{
    int commit_id;
    char cmd[MAX_ELEM_SIZE];
    char key[MAX_ELEM_SIZE];
    char value[MAX_ELEM_SIZE];
}sync_res;

typedef struct Commit_ok{
    int last_log_index;
}commit_ok;

typedef struct Set_log_res{
    int to_be_commit_index;
}set_log_res;
/*
typedef struct Vote{
    int message_sent_by;
}vote;

typedef struct Req_for_vote{
    int message_sent_by;
}req_for_vote;

*/


#define CONST_QUEUE_MSG_SIZE (sizeof(((Queue_node_data *)0)->event) +sizeof(((Queue_node_data *)0)->term) +\
				sizeof(((Queue_node_data *)0)->message_sent_by) +sizeof(((Queue_node_data *)0)->message_sent_to))

typedef struct queue_node_data{
    eventType event;
    int term;
    int message_sent_by;
    int message_sent_to;
    union {
        keep_alive_hb keep_alive_hb_msg;
        set_log_hb  set_log_hb_msg;
        sync_req sync_req_msg;
        sync_res sync_res_msg;
        commit_ok commit_ok_msg;
        set_log_res set_log_res_msg;
        //vote vote_msg;
        //req_for_vote req_for_vote_msg;

    }msg_data;

}Queue_node_data;


typedef struct configuration{
    int leader_timeout;
}configuration;


typedef struct Python_function{
    void (*add_to_log_DB)(int log_id,char* cmd,char* key,char* value);
    void (*update_DB)(char * DB_flag,char * key,char* value);
    void (*execute_log)(int last_log_index);
    char** (*get_log_by_diff)(int from,int to);
    void (*write_to_logger)(int logger_level,char* logger_info);
    void (*clear_log_from_log_id)(int);
}python_function;


typedef struct Shared_raft_data{ //contains all the shared structures needed

    struct{
        Queue_node_data queue[QUEUE_SIZE];
        int queue_start;
        int queue_end;
        int queue_elements_amount;
        pthread_mutex_t mutex_queue;
        sem_t sem_queue;
    }Raft_queue;

    state raft_state;//my state
    python_function python_functions;
    configuration raft_configuration;//raft configure - leader time out,
#if DEBUG_MODE ==1 
	int logger_fd;
#endif
}shared_raft_data;



shared_raft_data sharedRaftData;



#endif //RAFT_PROJECT_GENERAL_H
