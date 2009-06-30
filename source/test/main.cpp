// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 Nathan Turner <hailstone3 at sourceforge>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

// Uses a TestFactoryRegistry instead of including header file (see template)
// CPPUNIT_TEST_SUITE_REGISTRATION( TestSuite );

// Behind the scene, a static variable type of AutoRegisterSuite is declared.
// On construction, it will register a TestSuiteFactory into the TestFactoryRegistry.
// The TestSuiteFactory returns the TestSuite returned by ComplexNumber::suite().
// http://cppunit.sourceforge.net/doc/lastest/cppunit_cookbook.html

// code adapted from http://pantras.free.fr/articles/helloworld.html

#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

int main()
{
  //--- Create the event manager and test controller
  CPPUNIT_NS::TestResult controller;

  //--- Add a listener that collects test result
  CPPUNIT_NS::TestResultCollector result;
  controller.addListener( &result );

  //--- Add a listener that print dots as test run.
  CPPUNIT_NS::BriefTestProgressListener progress;
  controller.addListener( &progress );

  //--- Add the top suite to the test runner
  CPPUNIT_NS::TestRunner runner;
  runner.addTest( CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest() );
  runner.run( controller );

  system("Pause");

  return result.wasSuccessful() ? 0 : 1;
}