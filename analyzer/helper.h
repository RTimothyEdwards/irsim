
/*
 * Define NEED_HELPER if the system does not support SIGIO on sockets
 * and if we are not using pthreads.
 */

#if defined( ultrix )
#    if !defined( ULTRIX_VERSION )
#        define	NEED_HELPER
#    elif ULTRIX_VERSION <= 3
#        define NEED_HELPER
#    endif
#elif defined( hp9000s300 )
#    define	NEED_HELPER
#else
#    ifdef NO_SIGIO		/* this comes from CFLAGS at the top */
#	define	NEED_HELPER
#    endif
#endif
