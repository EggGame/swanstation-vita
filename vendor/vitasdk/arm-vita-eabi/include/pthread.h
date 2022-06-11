/* This is an implementation of the threads API of POSIX 1003.1-2001.
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-embedded (PTE) - POSIX Threads Library for embedded systems
 *      Copyright(C) 2008 Jason Schmidlapp
 *
 *      Contact Email: jschmidlapp@users.sourceforge.net
 *
 *
 *      Pthreads-embedded (PTE) - POSIX Threads Library for embedded systems
 *      Copyright(C) 2008 Jason Schmidlapp
 *
 *      Contact Email: jschmidlapp@users.sourceforge.net
 *
 *
 *      Based upon Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2005 Pthreads-win32 contributors
 *
 *      Contact Email: rpj@callisto.canberra.edu.au
 *
 *      The original list of contributors to the Pthreads-win32 project
 *      is contained in the file CONTRIBUTORS.ptw32 included with the
 *      source code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#if !defined( PTHREAD_H )
#define PTHREAD_H

#include <pte_types.h>

#include <sched.h>
#include <signal.h>

#define PTE_VERSION 2,8,0,0
#define PTE_VERSION_STRING "2, 8, 0, 0\0"

/* There are two implementations of cancel cleanup.
 * Note that pthread.h is included in both application
 * compilation units and also internally for the library.
 * The code here and within the library aims to work
 * for all reasonable combinations of environments.
 *
 * The two implementations are:
 *
 *   C
 *   C++
 *
 */

/*
 * Define defaults for cleanup code.
 * Note: Unless the build explicitly defines one of the following, then
 * we default to standard C style cleanup. This style uses setjmp/longjmp
 * in the cancelation and thread exit implementations and therefore won't
 * do stack unwinding if linked to applications that have it (e.g.
 * C++ apps). This is currently consistent with most/all commercial Unix
 * POSIX threads implementations.
 */
#if !defined( PTE_CLEANUP_CXX ) && !defined( PTE_CLEANUP_C )
# define PTE_CLEANUP_C
#endif

#undef PTE_LEVEL

#if defined(_POSIX_SOURCE)
#define PTE_LEVEL 0
/* Early POSIX */
#endif

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199309
#undef PTE_LEVEL
#define PTE_LEVEL 1
/* Include 1b, 1c and 1d */
#endif

#if defined(INCLUDE_NP)
#undef PTE_LEVEL
#define PTE_LEVEL 2
/* Include Non-Portable extensions */
#endif

#define PTE_LEVEL_MAX 3

#if !defined(PTE_LEVEL)
#define PTE_LEVEL PTE_LEVEL_MAX
/* Include everything */
#endif

/*
 * -------------------------------------------------------------
 *
 *
 * Module: pthread.h
 *
 * Purpose:
 *      Provides an implementation of PThreads based upon the
 *      standard:
 *
 *              POSIX 1003.1-2001
 *  and
 *    The Single Unix Specification version 3
 *
 *    (these two are equivalent)
 *
 *      in order to enhance code portability between Windows,
 *  various commercial Unix implementations, and Linux.
 *
 *      See the ANNOUNCE file for a full list of conforming
 *      routines and defined constants, and a list of missing
 *      routines and constants not defined in this implementation.
 *
 * Authors:
 *      There have been many contributors to this library.
 *      The initial implementation was contributed by
 *      John Bossom, and several others have provided major
 *      sections or revisions of parts of the implementation.
 *      Often significant effort has been contributed to
 *      find and fix important bugs and other problems to
 *      improve the reliability of the library, which sometimes
 *      is not reflected in the amount of code which changed as
 *      result.
 *      As much as possible, the contributors are acknowledged
 *      in the ChangeLog file in the source code distribution
 *      where their changes are noted in detail.
 *
 *      Contributors are listed in the CONTRIBUTORS file.
 *
 *      As usual, all bouquets go to the contributors, and all
 *      brickbats go to the project maintainer.
 *
 * Maintainer:
 *      The code base for this project is coordinated and
 *      eventually pre-tested, packaged, and made available by
 *
 *              Ross Johnson <rpj@callisto.canberra.edu.au>
 *
 * QA Testers:
 *      Ultimately, the library is tested in the real world by
 *      a host of competent and demanding scientists and
 *      engineers who report bugs and/or provide solutions
 *      which are then fixed or incorporated into subsequent
 *      versions of the library. Each time a bug is fixed, a
 *      test case is written to prove the fix and ensure
 *      that later changes to the code don't reintroduce the
 *      same error. The number of test cases is slowly growing
 *      and therefore so is the code reliability.
 *
 * Compliance:
 *      See the file ANNOUNCE for the list of implemented
 *      and not-implemented routines and defined options.
 *      Of course, these are all defined is this file as well.
 *
 * Web site:
 *      The source code and other information about this library
 *      are available from
 *
 *              http://sources.redhat.com/pthreads-win32/
 *
 * -------------------------------------------------------------
 */
#include "bits/posix_opt.h"
#include "sys/_pthreadtypes.h"
#include <time.h>

#include <setjmp.h>
#include <limits.h>

/*
 * Boolean values to make us independent of system includes.
 */
enum
{
  PTE_FALSE = 0,
  PTE_TRUE = (! PTE_FALSE)
};


typedef unsigned int pthread_t_;

    /*
     * ====================
     * ====================
     * POSIX Threads
     * ====================
     * ====================
     */

    enum
    {
      /*
       * pthread_setcancelstate paramters
       */
      PTHREAD_CANCEL_ENABLE         = 0,  /* Default */
      PTHREAD_CANCEL_DISABLE        = 1,

      /*
       * pthread_setcanceltype parameters
       */
      PTHREAD_CANCEL_ASYNCHRONOUS   = 0,
      PTHREAD_CANCEL_DEFERRED       = 1,  /* Default */

      /*
       * pthread_barrier_wait
       */
      PTHREAD_BARRIER_SERIAL_THREAD = -1
    };

    /*
     * ====================
     * ====================
     * Cancelation
     * ====================
     * ====================
     */
#define PTHREAD_CANCELED       ((void *) -1)

    /*
     * ====================
     * ====================
     * Once Key
     * ====================
     * ====================
     */
#define PTHREAD_ONCE_INIT       _PTHREAD_ONCE_INIT

    /*
     * ====================
     * ====================
     * Object initialisers
     * ====================
     * ====================
     */
#define PTHREAD_MUTEX_INITIALIZER _PTHREAD_MUTEX_INITIALIZER

    /*
     * Compatibility with LinuxThreads
     */
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP PTHREAD_ERRORCHECK_MUTEX_INITIALIZER

#define PTHREAD_COND_INITIALIZER _PTHREAD_COND_INITIALIZER

#define PTHREAD_RWLOCK_INITIALIZER _PTHREAD_RWLOCK_INITIALIZER

    typedef struct pte_cleanup_t pte_cleanup_t;

    typedef void (*  pte_cleanup_callback_t)(void *);

    struct pte_cleanup_t
      {
        pte_cleanup_callback_t routine;
        void *arg;
        struct pte_cleanup_t *prev;
      };

#ifdef PTE_CLEANUP_C

    pte_cleanup_t *  pte_pop_cleanup (int execute);

    void  pte_push_cleanup (pte_cleanup_t * cleanup,
                            void (*routine) (void *),
                            void *arg);
    /*
     * C implementation of PThreads cancel cleanup
     */

#define pthread_cleanup_push( _rout, _arg ) \
        { \
            pte_cleanup_t     _cleanup; \
            \
            pte_push_cleanup( &_cleanup, (pte_cleanup_callback_t) (_rout), (_arg) ); \
 
#define pthread_cleanup_pop( _execute ) \
            (void) pte_pop_cleanup( _execute ); \
        }

#else /* PTE_CLEANUP_C */

#ifdef PTE_CLEANUP_CXX

    /*
    * C++ version of cancel cleanup.
    * - John E. Bossom.
    */

    class PThreadCleanup
      {
        /*
        * PThreadCleanup
        *
        * Purpose
        *      This class is a C++ helper class that is
        *      used to implement pthread_cleanup_push/
        *      pthread_cleanup_pop.
        *      The destructor of this class automatically
        *      pops the pushed cleanup routine regardless
        *      of how the code exits the scope
        *      (i.e. such as by an exception)
        */
        pte_cleanup_callback_t cleanUpRout;
        void    *       obj;
        int             executeIt;

      public:
        PThreadCleanup() :
            cleanUpRout( 0 ),
            obj( 0 ),
            executeIt( 0 )
            /*
            * No cleanup performed
            */
        {
        }

        PThreadCleanup(
          pte_cleanup_callback_t routine,
          void    *       arg ) :
            cleanUpRout( routine ),
            obj( arg ),
            executeIt( 1 )
            /*
            * Registers a cleanup routine for 'arg'
            */
        {
        }

        ~PThreadCleanup()
        {
          if ( executeIt )
            {
              (void) (*cleanUpRout)( obj );
            }
        }

        void execute( int exec )
        {
          executeIt = exec;
        }
      };

    /*
    * C++ implementation of PThreads cancel cleanup;
    * This implementation takes advantage of a helper
    * class who's destructor automatically calls the
    * cleanup routine if we exit our scope weirdly
    */
#define pthread_cleanup_push( _rout, _arg ) \
        { \
            PThreadCleanup  cleanup((pte_cleanup_callback_t)(_rout), \
                                    (void *) (_arg) );

#define pthread_cleanup_pop( _execute ) \
            cleanup.execute( _execute ); \
        }

#else

#error ERROR [__FILE__, line __LINE__]: Cleanup type undefined.

#endif /* PTE_CLEANUP_CXX */

#endif /* PTE_CLEANUP_C */

    /*
     * ===============
     * ===============
     * Methods
     * ===============
     * ===============
     */
#ifdef __cplusplus
extern "C" {
#endif

    int  pthread_init (void);
    void  pthread_terminate (void);

    /*
     * PThread Attribute Functions
     */
    int  pthread_attr_init (pthread_attr_t * attr);

    int  pthread_attr_destroy (pthread_attr_t * attr);

    int  pthread_attr_getdetachstate (const pthread_attr_t * attr,
                                      int *detachstate);

    int  pthread_attr_getstackaddr (const pthread_attr_t * attr,
                                    void **stackaddr);

    int  pthread_attr_getstacksize (const pthread_attr_t * attr,
                                    size_t * stacksize);

    int  pthread_attr_setdetachstate (pthread_attr_t * attr,
                                      int detachstate);

    int  pthread_attr_setstackaddr (pthread_attr_t * attr,
                                    void *stackaddr);

    int  pthread_attr_setstacksize (pthread_attr_t * attr,
                                    size_t stacksize);

    int  pthread_attr_getschedparam (const pthread_attr_t *attr,
                                     struct sched_param *param);

    int  pthread_attr_setschedparam (pthread_attr_t *attr,
                                     const struct sched_param *param);

    int  pthread_attr_setschedpolicy (pthread_attr_t *,
                                      int);

    int  pthread_attr_getschedpolicy (pthread_attr_t *,
                                      int *);

    int  pthread_attr_setinheritsched(pthread_attr_t * attr,
                                      int inheritsched);

    int  pthread_attr_getinheritsched(pthread_attr_t * attr,
                                      int * inheritsched);

    int  pthread_attr_setscope (pthread_attr_t *,
                                int);

    int  pthread_attr_getscope (const pthread_attr_t *,
                                int *);

    int  pthread_attr_setstack(pthread_attr_t *attr,
                               void *stackaddr,
                               size_t stacksize);

    int  pthread_attr_getstack(const pthread_attr_t *attr,
                               void **stackaddr,
                               size_t *stacksize);

    int  pthread_attr_setguardsize(pthread_attr_t *attr,
                                   size_t guardsize);
    int  pthread_attr_getguardsize(const pthread_attr_t *attr,
                                   size_t *guardsize);

    /*
     * PThread Functions
     */
    int  pthread_create (pthread_t * tid,
                         const pthread_attr_t * attr,
                         void *(*start) (void *),
                         void *arg);

    int  pthread_detach (pthread_t tid);

    int  pthread_equal (pthread_t t1,
                        pthread_t t2);

    void  pthread_exit (void *value_ptr);

    int  pthread_join (pthread_t thread,
                       void **value_ptr);

    pthread_t  pthread_self (void);

    int  pthread_cancel (pthread_t thread);

    int  pthread_setcancelstate (int state,
                                 int *oldstate);

    int  pthread_setcanceltype (int type,
                                int *oldtype);

    void  pthread_testcancel (void);

    int  pthread_once (pthread_once_t * once_control,
                       void (*init_routine) (void));

    int  pthread_atfork(void (*prepare)(void),
                       void (*parent)(void),
                       void (*child)(void));

    /*
     * Thread Specific Data Functions
     */
    int  pthread_key_create (pthread_key_t * key,
                             void (*destructor) (void *));

    int  pthread_key_delete (pthread_key_t key);

    int  pthread_setspecific (pthread_key_t key,
                              const void *value);

    void *  pthread_getspecific (pthread_key_t key);


    /*
     * Mutex Attribute Functions
     */
    int  pthread_mutexattr_init (pthread_mutexattr_t * attr);

    int  pthread_mutexattr_destroy (pthread_mutexattr_t * attr);

    int  pthread_mutexattr_getpshared (const pthread_mutexattr_t
                                       * attr,
                                       int *pshared);

    int  pthread_mutexattr_setpshared (pthread_mutexattr_t * attr,
                                       int pshared);

    int  pthread_mutexattr_settype (pthread_mutexattr_t * attr, int kind);
    int  pthread_mutexattr_gettype (pthread_mutexattr_t * attr, int *kind);

    /*
     * Barrier Attribute Functions
     */
    int  pthread_barrierattr_init (pthread_barrierattr_t * attr);

    int  pthread_barrierattr_destroy (pthread_barrierattr_t * attr);

    int  pthread_barrierattr_getpshared (const pthread_barrierattr_t
                                         * attr,
                                         int *pshared);

    int  pthread_barrierattr_setpshared (pthread_barrierattr_t * attr,
                                         int pshared);

    /*
     * Mutex Functions
     */
    int  pthread_mutex_init (pthread_mutex_t * mutex,
                             const pthread_mutexattr_t * attr);

    int  pthread_mutex_destroy (pthread_mutex_t * mutex);

    int  pthread_mutex_lock (pthread_mutex_t * mutex);

    int  pthread_mutex_timedlock(pthread_mutex_t *mutex,
                                 const struct timespec *abstime);

    int  pthread_mutex_trylock (pthread_mutex_t * mutex);

    int  pthread_mutex_unlock (pthread_mutex_t * mutex);

    /*
     * Spinlock Functions
     */
    int  pthread_spin_init (pthread_spinlock_t * lock, int pshared);

    int  pthread_spin_destroy (pthread_spinlock_t * lock);

    int  pthread_spin_lock (pthread_spinlock_t * lock);

    int  pthread_spin_trylock (pthread_spinlock_t * lock);

    int  pthread_spin_unlock (pthread_spinlock_t * lock);

    /*
     * Barrier Functions
     */
    int  pthread_barrier_init (pthread_barrier_t * barrier,
                               const pthread_barrierattr_t * attr,
                               unsigned int count);

    int  pthread_barrier_destroy (pthread_barrier_t * barrier);

    int  pthread_barrier_wait (pthread_barrier_t * barrier);

    /*
     * Condition Variable Attribute Functions
     */
    int  pthread_condattr_init (pthread_condattr_t * attr);

    int  pthread_condattr_destroy (pthread_condattr_t * attr);

    int  pthread_condattr_getpshared (const pthread_condattr_t * attr,
                                      int *pshared);

    int  pthread_condattr_setpshared (pthread_condattr_t * attr,
                                      int pshared);

    int  pthread_condattr_getclock (const pthread_condattr_t * attr,
                                    clockid_t *clock_id);

    int  pthread_condattr_setclock (pthread_condattr_t * attr,
                                    clockid_t clock_id);

    /*
     * Condition Variable Functions
     */
    int  pthread_cond_init (pthread_cond_t * cond,
                            const pthread_condattr_t * attr);

    int  pthread_cond_destroy (pthread_cond_t * cond);

    int  pthread_cond_wait (pthread_cond_t * cond,
                            pthread_mutex_t * mutex);

    int  pthread_cond_timedwait (pthread_cond_t * cond,
                                 pthread_mutex_t * mutex,
                                 const struct timespec *abstime);

    int  pthread_cond_signal (pthread_cond_t * cond);

    int  pthread_cond_broadcast (pthread_cond_t * cond);

    /*
     * Scheduling
     */
    int  pthread_setschedparam (pthread_t thread,
                                int policy,
                                const struct sched_param *param);

    int  pthread_getschedparam (pthread_t thread,
                                int *policy,
                                struct sched_param *param);

    int  pthread_setconcurrency (int);

    int  pthread_getconcurrency (void);

    /*
     * Read-Write Lock Functions
     */
    int  pthread_rwlock_init(pthread_rwlock_t *lock,
                             const pthread_rwlockattr_t *attr);

    int  pthread_rwlock_destroy(pthread_rwlock_t *lock);

    int  pthread_rwlock_tryrdlock(pthread_rwlock_t *);

    int  pthread_rwlock_trywrlock(pthread_rwlock_t *);

    int  pthread_rwlock_rdlock(pthread_rwlock_t *lock);

    int  pthread_rwlock_timedrdlock(pthread_rwlock_t *lock,
                                    const struct timespec *abstime);

    int  pthread_rwlock_wrlock(pthread_rwlock_t *lock);

    int  pthread_rwlock_timedwrlock(pthread_rwlock_t *lock,
                                    const struct timespec *abstime);

    int  pthread_rwlock_unlock(pthread_rwlock_t *lock);

    int  pthread_rwlockattr_init (pthread_rwlockattr_t * attr);

    int  pthread_rwlockattr_destroy (pthread_rwlockattr_t * attr);

    int  pthread_rwlockattr_getpshared (const pthread_rwlockattr_t * attr,
                                        int *pshared);

    int  pthread_rwlockattr_setpshared (pthread_rwlockattr_t * attr,
                                        int pshared);

#if PTE_LEVEL >= PTE_LEVEL_MAX - 1
#include <signal.h>
    /*
     * Signal Functions. Should be defined in <signal.h> but we might
     * already have signal.h that don't define these.
     */
    int  pthread_kill(pthread_t thread, int sig);

    int  pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);

    /*
     * Non-portable functions
     */

    /*
     * Compatibility with Linux.
     */
    int  pthread_mutexattr_setkind_np(pthread_mutexattr_t * attr,
                                      int kind);
    int  pthread_mutexattr_getkind_np(pthread_mutexattr_t * attr,
                                      int *kind);

    /*
     * Possibly supported by other POSIX threads implementations
     */
    int  pthread_delay_np (struct timespec * interval);
    int  pthread_num_processors_np(void);

    int pthread_setaffinity_np(pthread_t thread, size_t cpusetsize,
                                  const cpu_set_t *cpuset);
    int pthread_getaffinity_np(pthread_t thread, size_t cpusetsize,
                                  cpu_set_t *cpuset);

    /*
     * Register a system time change with the library.
     * Causes the library to perform various functions
     * in response to the change. Should be called whenever
     * the application's top level window receives a
     * WM_TIMECHANGE message. It can be passed directly to
     * pthread_create() as a new thread if desired.
     */
    void *  pthread_timechange_handler_np(void *);

#endif /*PTE_LEVEL >= PTE_LEVEL_MAX - 1 */

/* We deal here with a gcc issue for posix threading on vita.
   We would need to change here gcc's gthr-posix.h header, but this
   got rejected.  So we deal it within this header.  */
#ifdef _GTHREAD_USE_MUTEX_INIT_FUNC
#undef _GTHREAD_USE_MUTEX_INIT_FUNC
#endif
#define _GTHREAD_USE_MUTEX_INIT_FUNC 1

#ifdef __cplusplus
}
#endif

    /*
     * Some compiler environments don't define some things.
     */
#  define _ftime ftime
#  define _timeb timeb

#ifdef __cplusplus
    /*
     * Internal exceptions
     */
    class pte_exception {};
    class pte_exception_cancel : public pte_exception {};
    class pte_exception_exit   : public pte_exception {};

#endif


#ifdef PTE_CXX_EXCEPTIONS

    /*
     * Redefine the C++ catch keyword to ensure that applications
     * propagate our internal exceptions up to the library's internal handlers.
     */
#define catch( E ) \
        catch( pte_exception & ) { throw; } \
        catch( E )

#endif /* ! PTE_CXX_EXCEPTIONS */

#undef PTE_LEVEL
#undef PTE_LEVEL_MAX

#endif /* PTHREAD_H */
