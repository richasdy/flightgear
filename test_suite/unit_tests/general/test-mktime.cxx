// test the systems mktime() function

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <math.h>
#include <stdio.h>
#include <time.h>

#ifdef HAVE_SYS_TIMEB_H
#  include <sys/types.h>
#  include <sys/timeb.h> // for ftime() and struct timeb
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>    // for gettimeofday()
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // for get/setitimer, gettimeofday, struct timeval
#endif

#include "test-mktime.hxx"

#define LST_MAGIC_TIME_1998 890481600

// For now we assume that if daylight is not defined in
// /usr/include/time.h that we have a machine with a BSD behaving
// mktime()
#if !defined(HAVE_DAYLIGHT)
#  define MK_TIME_IS_GMT 1
#endif


// Fix up timezone if using ftime()
long int fix_up_timezone( long int timezone_orig ) {
#if !defined( HAVE_GETTIMEOFDAY ) && defined( HAVE_FTIME )
    // ftime() needs a little extra help finding the current timezone
    struct timeb current;
    ftime(&current);
    return( current.timezone * 60 );
#else
    return( timezone_orig );
#endif
}


// A printout function suitable for the test suite to replace printf().
int printout(const char *text, ...)
{
    std::cout << text;
    return 0;
}


// Return time_t for Sat Mar 21 12:00:00 GMT
//
// I believe the mktime() has a SYSV vs. BSD behavior difference.
//
// The BSD style mktime() is nice because it returns its result
// assuming you have specified the input time in GMT
//
// The SYSV style mktime() is a pain because it returns its result
// assuming you have specified the input time in your local timezone.
// Therefore you have to go to extra trouble to convert back to GMT.
//
// If you are having problems with incorrectly positioned astronomical
// bodies, this is a really good place to start looking.

time_t get_start_gmt(int year) {
    struct tm mt;

    mt.tm_mon = 2;
    mt.tm_mday = 21;
    mt.tm_year = year;
    mt.tm_hour = 12;
    mt.tm_min = 0;
    mt.tm_sec = 0;
    mt.tm_isdst = -1; // let the system determine the proper time zone

#if defined( HAVE_TIMEGM ) 
    return ( timegm(&mt) );
#elif defined( MK_TIME_IS_GMT )
    return ( mktime(&mt) );
#else // ! defined ( MK_TIME_IS_GMT )

    // timezone seems to work as a proper offset for Linux & Solaris
#  if defined( __linux__ ) || defined(__sun) || defined( __CYGWIN__ )
#   define TIMEZONE_OFFSET_WORKS 1
#  endif

#if defined(__CYGWIN__)
#define TIMEZONE _timezone
#else
#define TIMEZONE timezone
#endif

    time_t start = mktime(&mt);

    printout("start1 = %ld\n", start);
    printout("start2 = %s", ctime(&start));
    printout("(tm_isdst = %d)\n", mt.tm_isdst);

    TIMEZONE = fix_up_timezone( TIMEZONE );
	
#  if defined( TIMEZONE_OFFSET_WORKS )
    printout("start = %ld, timezone = %ld\n", start, TIMEZONE);
    return( start - TIMEZONE );
#  else // ! defined( TIMEZONE_OFFSET_WORKS )

    daylight = mt.tm_isdst;
    if ( daylight > 0 ) {
	daylight = 1;
    } else if ( daylight < 0 ) {
	printout("OOOPS, problem in fg_time.cxx, no daylight savings info.\n");
    }

    long int offset = -(TIMEZONE / 3600 - daylight);

    printout("  Raw time zone offset = %ld\n", TIMEZONE);
    printout("  Daylight Savings = %d\n", daylight);
    printout("  Local hours from GMT = %ld\n", offset);
    
    long int start_gmt = start - TIMEZONE + (daylight * 3600);
    
    printout("  March 21 noon (CST) = %ld\n", start);

    return ( start_gmt );
#  endif // ! defined( TIMEZONE_OFFSET_WORKS )
#endif // ! defined ( MK_TIME_IS_GMT )
}


// The original test-mktime test.
void MktimeTests::testMktime()
{
    time_t start_gmt;

    start_gmt = get_start_gmt(98);


    if ( start_gmt == LST_MAGIC_TIME_1998 ) {
	printout("Time test = PASSED\n\n");
#ifdef HAVE_TIMEGM
	printout("You have timegm() which is just like mktime() except that\n");
	printout("it explicitely expects input in GMT ... lucky you!\n");
#elif MK_TIME_IS_GMT
	printout("You don't seem to have timegm(), but mktime() seems to\n");
	printout("assume input is GMT on your system ... I guess that works\n");
#else
	printout("mktime() assumes local time zone on your system, but we can\n");
	printout("compensate just fine.\n");
#endif
    } else {
	printout("Time test = FAILED\n\n");
	printout("There is likely a problem with mktime() on your system.\n");
        printout("This will cause the sun/moon/stars/planets to be in the\n");
	printout("wrong place in the sky and the rendered time of day will be\n");
	printout("incorrect.\n\n");
	printout("Please report this to flightgear-devel@lists.sourceforge.net so we can work to fix\n");
	printout("the problem on your platform.\n");
    }
    CPPUNIT_ASSERT_EQUAL(LST_MAGIC_TIME_1998, (int)start_gmt);
}
