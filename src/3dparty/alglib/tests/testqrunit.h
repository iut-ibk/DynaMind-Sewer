
#ifndef _testqrunit_h
#define _testqrunit_h

#include "ap.h"
#include "ialglib.h"

#include "reflections.h"
#include "qr.h"


/*************************************************************************
Main unittest subroutine
*************************************************************************/
bool testqr(bool silent);


/*************************************************************************
Silent unit test
*************************************************************************/
bool testqrunit_test_silent();


/*************************************************************************
Unit test
*************************************************************************/
bool testqrunit_test();


#endif
