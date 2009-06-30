// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 Author <email>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef TEST_SUITE_TEMPLATE_H
#define TEST_SUITE_TEMPLATE_H

// -- insert headers of class you are testing -- //

#include <cppunit/extensions/HelperMacros.h>

class TestSuiteTemplate : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE( TestSuiteTemplate );
  // -- insert test names here that correspond with the test methods -- //
  //CPPUNIT_TEST( testMethod );
  //CPPUNIT_TEST_EXCEPTION( testExceptionMethod, TheException );
  CPPUNIT_TEST_SUITE_END();

private:
  // -- insert instance variables -- //
public:
  void setUp()
  {
    // -- insert set up code -- //
  }

  void tearDown()
  {
    // -- insert tear down code -- //
  }

public:
  //-- Normal Cases

  //-- Corner Cases

  //-- Exception Cases
};

#endif