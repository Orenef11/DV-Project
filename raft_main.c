
#include "raft_main.h"


//main.c
void *run_multicast_listener(void * args){
    Queue_node_data new_node;
    int is_valid;
    while(1){
        is_valid = !get_raft_message(&new_node);
#if DEBUG_MODE == 1
        WRITE_TO_LOGGER(DEBUG_LEVEL,"got new message and ids as followed:",INT_VALUES,2,LOG(new_node.message_sent_by),
                        LOG(sharedRaftData.raft_state.server_id));
#endif
        if(is_valid && is_relevant_message(&new_node)){
            push_queue(&new_node);
        }
    }
}

//main.c
void* raft_manager(void * args){
    Queue_node_data new_node;
    while(1){
        sem_wait(&sharedRaftData.Raft_queue.sem_queue);
        pop_queue(&new_node);
        operate_machine_state(&new_node);
    }
}

//c calls py func
//set the callback function in the shared_raft_data struct
void transfer_callback_function(void (*add_to_log_DB)(int log_id,char* cmd,char* key,char* value),
							void (*update_DB)(char * DB_flag,char * key,char* value),
                               char** (*get_log_by_diff)(int from,int to),
							   void (*write_to_logger)(int logger_level,char * logger_info),
							void (*execute_log)(int),
							void (*clear_log_from_log_id)(int))
{
    sharedRaftData.python_functions.add_to_log_DB         = add_to_log_DB;
    sharedRaftData.python_functions.update_DB             = update_DB;
    sharedRaftData.python_functions.get_log_by_diff       = get_log_by_diff;
    sharedRaftData.python_functions.write_to_logger       = write_to_logger;
    sharedRaftData.python_functions.execute_log           = execute_log;
    sharedRaftData.python_functions.clear_log_from_log_id = clear_log_from_log_id;
}

//utils.c
//every exit logic shuold be here
int exit_raft(int exit_rv){
	#if DEBUG_MODE == 1
	WRITE_TO_LOGGER(DEBUG_LEVEL,"exit from raft...",NO_VALUES,0);
	if(exit_rv){
		puts(strerror(exit_rv));
		WRITE_TO_LOGGER(DEBUG_LEVEL,"ERROR Message",CHARS_VALUES,1,LOG(strerror(errno)));
	}
	close(sharedRaftData.logger_fd);
#endif
	exit(exit_rv);
}

//utils.c
//close raft server from CLI  - handler for SIGSTOP
//every exit logic shuold be here
#if DEBUG_MODE == 1
int signal_cnt =0;
#endif
void signal_handler(int sig){
	//puts("signal");
	if(sig == SIGINT){
		exit_raft(0);
	}
	else if(sig == SIGALRM){
		time_out_hendler(sig);
	}
}

//utils.c
//set handlers for exit and timeout events
int init_sig_handler(){
	int rv = 0;
    //handler for raft timeout event
    struct sigaction sa_timeout;
    //handler for exit command from CLI
    struct sigaction sa_exit;
    sigset_t set;
	sigemptyset(&set);
	pthread_sigmask(SIG_SETMASK, &set, NULL);
    memset(&sa_timeout, 0, sizeof (sa_timeout));
    memset(&sa_exit, 0, sizeof (sa_exit));
    sa_timeout.sa_handler = &signal_handler;
    sa_exit.sa_handler = &signal_handler;
    rv |=sigaction(SIGALRM, &sa_timeout, NULL);
    rv |=sigaction(SIGINT, &sa_exit, NULL);
    return  rv ;
}

//main.c / ?init_logic.c?
void set_raft_data(int id,int members_num,int leader_timeout,void(*set_callback_function)(void)){
    //set sharedRaftData.python_functions
    //call to python function that will call to transfer_callback_function and set the function pointer
    set_callback_function();

    //set sharedRaftData.raft_state
    sharedRaftData.raft_state.current_state          = FOLLOWER;
    sharedRaftData.raft_state.server_id              = id;
    sharedRaftData.raft_state.members_amount         = members_num;
    sharedRaftData.raft_state.timeout                = calculate_raft_rand_timeout();
    sharedRaftData.raft_state.leader_id              = 0;

    //set sharedRaftData.raft_configuration
    sharedRaftData.raft_configuration.leader_timeout = leader_timeout;

}

//main.c / ?init_logic.c?
//initialize parameters (socket etc)
int init_raft(char* raft_ip,int raft_port,int id,int members_num,int leader_timeout,void(*set_callback_function)(void)){
	int rv;
	//for rand func
    srand(time(NULL));
	set_raft_data(id,members_num,leader_timeout,set_callback_function);
	//set_raft_data should be called before  init_state_functions_handler
    init_state_functions_handler();
    if((rv=init_multicast_message(raft_ip,raft_port))){
		exit_raft(rv);
    }

    if((rv=init_raft_queue())){
        exit_raft(rv);
    }

    create_alarm_timer(sharedRaftData.raft_state.timeout);
    if((rv=init_sig_handler())){
		exit_raft(rv);
    }
    //INIT DB VALUES
    update_DB(DB_STATUS,LEADER_ID, 0);
    update_DB(DB_STATUS,COMMIT_INDEX, 0 );
    update_DB(DB_STATUS, LAST_APPLIED, 0);
    update_DB(DB_STATUS, STATUS, FOLLOWER_VALUE);
    update_DB(DB_STATUS, TERM, 0);

#if DEBUG_MODE == 1
	WRITE_TO_LOGGER(DEBUG_LEVEL,"init succeed",NO_VALUES,0);
#endif
    return 0;
}

//main.c
void create_new_log_command(int log_id,char * cmd,char * key, char * value,Queue_node_data *new_node_memory){
	new_node_memory->event = PY_SEND_LOG;
	new_node_memory->term = sharedRaftData.raft_state.term;
	new_node_memory->message_sent_by = sharedRaftData.raft_state.server_id;
	new_node_memory->msg_data.set_log_hb_msg.commit_id = log_id;
	sprintf(new_node_memory->msg_data.set_log_hb_msg.cmd,"%s",cmd);
	sprintf(new_node_memory->msg_data.set_log_hb_msg.key,"%s",key);
	sprintf(new_node_memory->msg_data.set_log_hb_msg.value,"%s",value == NULL ? "":value);
}

//main.c
void start_commit_proccess(int log_id,char * cmd,char * key, char * value){
	puts("11111111111111111dwedwdwdw");
    Queue_node_data new_node;
    create_new_log_command(log_id,cmd,key,value,&new_node);
    push_queue(&new_node);
}

//main.c
//pyhton start raft process, call init_raft()
int run_raft(char* raft_ip,int raft_port,int server_id,int members_num,
                int leader_timeout,void(*set_callback_function)(void))
{
    pthread_t server_thread,raft_handler_thread;
    init_raft(raft_ip,raft_port,server_id,members_num,leader_timeout,set_callback_function);
    //init raft must be callded before WRITE_TO_LOGGER
#if DEBUG_MODE == 1
	WRITE_TO_LOGGER(DEBUG_LEVEL,"start raft...",CHARS_VALUES,1,
			LOG(raft_ip));
	WRITE_TO_LOGGER(DEBUG_LEVEL,"start raft...",INT_VALUES,4,
			LOG(raft_port),LOG(server_id),LOG(members_num),LOG(leader_timeout));
#endif
    pthread_create(&raft_handler_thread, NULL, raft_manager, NULL);
    pthread_create(&server_thread, NULL, run_multicast_listener, NULL);
#if DEBUG_MODE == 1
	WRITE_TO_LOGGER(DEBUG_LEVEL,"run raft...",NO_VALUES,0);
#endif
    pthread_join(raft_handler_thread, NULL);
    pthread_join(server_thread, NULL);
#if DEBUG_MODE == 1
	WRITE_TO_LOGGER(DEBUG_LEVEL,"exit raft",NO_VALUES,0);
#endif
    return 0;
}


