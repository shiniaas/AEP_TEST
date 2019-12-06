/*
	This is to test the latency and bandwidth of AEP
*/
#include "lat_bw.h"

int main(int argc, char* argv[])
{
	parameter par;
	char file[128] = "/home/tony/Desktop/pmemdir/a";
	int is_pmem;
        size_t mapped_len;
	uint64_t num;
	int i;
	char opt[10];

	// default value
	par.mem_size = 16L * 1024 * 1024 * 1024;
	par.block_size = 512;
	par.block_num = par.mem_size / par.block_size;
	par.opt = READ;
	par.num_thread = 1;
	par.pmem_file_path = file;
	par.init = false;
	par.isflush = false;
	par.numa = -1;
	par.pmem = NULL;

	// get parameter from the command line
	for (i = 0; i < argc; i++) {
		if (sscanf(argv[i], "--mem_size=%lu", &num) == 1) {
			par.mem_size = num;
		} else if (sscanf(argv[i], "--block_size=%lu", &num) == 1) {
			par.block_size = num;
		} else if (sscanf(argv[i], "--rw=%s", opt) == 1) {
			if (strcmp(opt, "read") == 0) {
				par.opt = READ;
			} else if (strcmp(opt, "randread") == 0) {
				par.opt = RAND_READ;
			} else if (strcmp(opt, "write") == 0) {
				par.opt = WRITE;
			} else if (strcmp(opt, "randwrite") == 0) {
				par.opt = RAND_WRITE;
			} else {
				printf("rw=%s not supported\n", opt);
				return -1;
			}
		} else if (sscanf(argv[i], "--num_thread=%lu", &num) == 1) {
			par.num_thread = num;
		} else if (sscanf(argv[i], "--file=%s", file) == 1) {
			par.pmem_file_path = file;
		} else if (strcmp(argv[i], "--init") == 0) {
			par.init = true;
		} else if (strcmp(argv[i], "--flush") == 0) {
			par.isflush = true;
		} else if (sscanf(argv[i], "--numa=%lu", &num) == 1) {
			par.numa = (num == 0) ? 0 : 1;
		} 
		else {
			printf("%s not supported\n", argv[i]);
			return -1;
		}
	}

	par.block_num = par.mem_size / par.block_size;

	if ( (par.pmem = (uint64_t*)pmem_map_file(par.pmem_file_path, par.mem_size, 
			PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem)) == NULL ) {
		perror("pmem_map_file");
		exit(1);
	}
	
	srand(time(NULL));
	if (par.init) {
		printf("start to init with random value\n");
		assert(par.mem_size % sizeof(uint64_t) == 0);
		for (i = 0; i < par.mem_size / sizeof(uint64_t); i++) {
			par.pmem[i] = rand();
		}
	}

	run_test(&par);
	return 0;
}
