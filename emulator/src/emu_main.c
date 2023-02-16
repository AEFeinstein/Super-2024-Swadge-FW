#include <freertos/task.h>

#ifdef __linux__
void init_crashSignals(void);
void signalHandler_crash(int signum, siginfo_t* si, void* vcontext);
#endif

int main(int argc, char** argv)
{
#ifdef __linux__
    init_crashSignals();
#endif
}

void taskYIELD(void)
{
    
}

#ifdef __linux__

/**
 * @brief Initialize a crash handler, only for Linux
 */
void init_crashSignals(void)
{
    const int sigs[] = {SIGSEGV, SIGBUS, SIGILL, SIGSYS, SIGABRT, SIGFPE, SIGIOT, SIGTRAP};
    for(int i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
    {
        struct sigaction action;
        memset(&action, 0, sizeof(struct sigaction));
        action.sa_flags = SA_SIGINFO;
        action.sa_sigaction = signalHandler_crash;
        sigaction(sigs[i], &action, NULL);
    }
}

/**
 * @brief Print a backtrace when a crash is caught, only for Linux
 * 
 * @param signum 
 * @param si 
 * @param vcontext 
 */
void signalHandler_crash(int signum, siginfo_t* si, void* vcontext)
{
	char msg[128] = {'\0'};
	ssize_t result;

    char fname[64] = {0};
    sprintf(fname, "crash-%ld.txt", time(NULL));
    int dumpFileDescriptor = open(fname, O_RDWR | O_CREAT | O_TRUNC,
									S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if(-1 != dumpFileDescriptor)
	{
		snprintf(msg, sizeof(msg), "Signal %d received!\nsigno: %d, errno %d\n, code %d\n", signum, si->si_signo, si->si_errno, si->si_code);
		result = write(dumpFileDescriptor, msg, strnlen(msg, sizeof(msg)));
		(void)result;

        memset(msg, 0, sizeof(msg));
        for(int i = 0; i < __SI_PAD_SIZE; i++)
        {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%02X", si->_sifields._pad[i]);
            tmp[sizeof(tmp)-1] = '\0';
            strncat(msg, tmp, sizeof(msg) - strlen(msg) - 1);
        }
        strncat(msg, "\n", sizeof(msg) - strlen(msg) - 1);
		result = write(dumpFileDescriptor, msg, strnlen(msg, sizeof(msg)));
		(void)result;
        
        // Print backtrace
        void *array[128];
		size_t size = backtrace(array, (sizeof(array) / sizeof(array[0])));
		backtrace_symbols_fd(array, size, dumpFileDescriptor);
		close(dumpFileDescriptor);
	}

	// Exit
	_exit(1);
}
#endif
