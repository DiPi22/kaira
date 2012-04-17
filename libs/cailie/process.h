
#ifndef CAILIE_PROCESS_H
#define CAILIE_PROCESS_H

#include <pthread.h>
#include <vector>
#include "messages.h"
#include "net.h"
#include "packing.h"
#include "logging.h"

#ifdef CA_MPI
#include "mpi_requests.h"
#endif

#define CA_RESERVED_PREFIX sizeof(CaTokens)

#define CA_TAG_TOKENS 0
#define CA_TAG_SERVICE 1

class CaProcess;
class CaThread;
struct CaServiceMessage;

struct CaTokens {
	int place_index;
	int net_id;
	int tokens_count;
};

#ifdef CA_SHMEM
class CaPacket {
	public:
	int tag;
	void *data;

	CaPacket *next;
};
#endif

class CaProcess {
	public:
		CaProcess(int process_id, int process_count, int threads_count, int defs_count, CaNetDef **defs);
		virtual ~CaProcess();
		void start();
		void join();
		void inform_new_network(CaNet *net, CaThread *thread);
		void inform_halt_network(int net_id, CaThread *thread);
		void send_barriers(pthread_barrier_t *barrier1, pthread_barrier_t *barrier2);

		int get_threads_count() const { return threads_count; }
		int get_process_count() const { return process_count; }
		int get_process_id() const { return process_id; }
		void write_reports(FILE *out) const;
		void fire_transition(int transition_id, int instance_id);

		void quit_all();
		void quit() { quit_flag = true; }
		void halt(CaThread *thread, CaNet *net);

		void start_logging(const std::string &logname);
		void stop_logging();

		CaNet * spawn_net(CaThread *thread, int def_index, int id, CaNet *parent_net, bool globally);

		int new_net_id();

		CaThread *get_thread(int id);

		bool quit_flag;

		void multisend(int target, CaNet * net, int place, int tokens_count, const CaPacker &packer);
		void multisend_multicast(const std::vector<int> &targets, CaNet *net, int place, int tokens_count, const CaPacker &packer);

		void process_service_message(CaThread *thread, CaServiceMessage *smsg);
		void process_packet(CaThread *thread, int tag, void *data);
		int process_packets(CaThread *thread);

		#ifdef CA_SHMEM
		void add_packet(int tag, void *data);
		#endif

		void broadcast_packet(int tag, void *data, size_t size, int exclude = -1);
	protected:

		void autohalt_check(CaNet *net);

		int process_id;
		int process_count;
		int threads_count;
		int defs_count;
		CaNetDef **defs;
		CaThread *threads;
		int id_counter;
		pthread_mutex_t counter_mutex;

		#ifdef CA_SHMEM
		pthread_mutex_t packet_mutex;
		CaPacket *packets;
		#endif

		#ifdef CA_MPI
		CaMpiRequests requests;
		#endif
};

class CaThread {
	public:
		CaThread();
		~CaThread();
		int get_id() { return id; }
		void start();
		void join();
		void run_scheduler();

		int get_process_id() { return process->get_process_id(); }
		int get_process_count() { return process->get_process_count(); }
		int get_threads_count() { return process->get_threads_count(); }

		void add_message(CaThreadMessage *message);
		bool process_thread_messages();
		int process_messages();
		void clean_thread_messages();
		void process_message(CaThreadMessage *message);
		void quit_all();

		/* This function always sends thread message, it does not free net instantly
			this is the reason why first argument is NULL */
		void halt(CaNet *net) { process->halt(NULL, net); }

		void send(int target, CaNet *net, int place, const CaPacker &packer) {
			process->multisend(target, net, place, 1, packer);
		}
		void multisend(int target, CaNet *net, int place, int tokens_count, const CaPacker &packer) {
			process->multisend(target, net, place, tokens_count, packer);
		}
		void send_multicast(const std::vector<int> &targets, CaNet *net, int place, const CaPacker &packer) {
			process->multisend_multicast(targets, net, place, 1, packer);
		}
		void multisend_multicast(const std::vector<int> &targets, CaNet *net, int place, int tokens_count, const CaPacker &packer) {
			process->multisend_multicast(targets, net, place, tokens_count, packer);
		}
		CaProcess * get_process() const { return process; }

		void init_log(const std::string &logname);
		void close_log() { if (logger) { delete logger; logger = NULL; } }

		CaNet * spawn_net(int def_index, CaNet *parent_net);
		CaNet * get_net(int id);
		CaNet * remove_net(int id);
		/*
		void log_transition_start(CaUnit *unit, int transition_id) {
			if (logger) { logger->log_transition_start(unit, transition_id); }
		}

		void log_transition_end(CaUnit *unit, int transition_id) {
			if (logger) { logger->log_transition_end(unit, transition_id); }
		}

		void log_token_add(CaUnit *unit, int place_id, const std::string &token_string) {
			if (logger) { logger->log_token_add(unit, place_id, token_string); }
		}

		void log_token_remove(CaUnit *unit, int place_id, const std::string &token_string) {
			if (logger) { logger->log_token_remove(unit, place_id, token_string); }
		}

		void log_unit_status(CaUnit *unit, int def_id) {
		//	if (logger) { unit->log_status(logger, process->get_def(def_id)); }
		}
		*/

		void add_network(CaNet *net) {
			nets.push_back(net);
		}

		/*
		void start_logging(const std::string &logname) { process->start_logging(logname); }
		void stop_logging() { process->stop_logging(); }
		*/

		int get_nets_count() { return nets.size(); }
		const std::vector<CaNet*> & get_nets() { return nets; }

		void set_process(CaProcess *process, int id) { this->process = process; this->id = id; }

		CaNet *last_net() { return nets[nets.size() - 1]; }

	protected:
		CaProcess *process;
		pthread_t thread;
		pthread_mutex_t messages_mutex;
		CaThreadMessage *messages;
		std::vector<CaNet*> nets;
		int id;

		#ifdef CA_MPI
		CaMpiRequests requests;
		#endif

		CaLogger *logger;
};

#endif