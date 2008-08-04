/** \file testrunner.cc
 *  \brief Run multiple tests for different backends.
 */
/* Copyright 2008 Lemur Consulting Ltd
 * Copyright 2008 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <config.h>

#include "testrunner.h"

#include "testsuite.h"
#include "backendmanager.h"
#include "backendmanager_chert.h"
#include "backendmanager_flint.h"
#include "backendmanager_inmemory.h"
#include "backendmanager_multi.h"
#include "backendmanager_remoteprog.h"
#include "backendmanager_remotetcp.h"

#include "stringutils.h"
#include <iostream>

using namespace std;

BackendManager * backendmanager;

/** A description of the properties which a particular backend supports.
 */
struct BackendProperties {
    const char * name;
    const char * properties;
};

/** A list of the properties of each backend.
 */
static BackendProperties backend_properties[] = {
    { "none", "" },
    { "inmemory", "backend,positional,writable,valuestats,inmemory" },
    { "chert", "backend,transactions,positional,writable,spelling,metadata,"
	       "synonyms,replicas,valuestats,chert" },
    { "flint", "backend,transactions,positional,writable,spelling,metadata,"
	       "synonyms,replicas,flint" },
    { "multi_flint", "backend,positional,multi" },
    { "multi_chert", "backend,positional,valuestats,multi" },
    { "remoteprog_flint", "backend,remote,transactions,positional,writable" },
    { "remotetcp_flint", "backend,remote,transactions,positional,writable" },
    { "remoteprog_chert", "backend,remote,transactions,positional,valuestats,writable" },
    { "remotetcp_chert", "backend,remote,transactions,positional,valuestats,writable" },
    { NULL, NULL }
};

TestRunner::~TestRunner() { }

void
TestRunner::set_properties(const string & properties)
{
    // Clear the flags
    backend = false;
    remote = false;
    transactions = false;
    positional = false;
    writable = false;
    multi = false;
    spelling = false;
    synonyms = false;
    metadata = false;
    replicas = false;
    valuestats = false;
    inmemory = false;
    flint = false;
    chert = false;

    // Read the properties specified in the string
    string::size_type pos = 0;
    string::size_type comma = 0;
    while (pos != string::npos) {
	comma = properties.find(',', pos + 1);
	string propname = properties.substr(pos, comma - pos);

	// Set the flags according to the property.
	if (propname.empty()) {}
	else if (propname == "backend")
	    backend = true;
	else if (propname == "remote")
	    remote = true;
	else if (propname == "transactions")
	    transactions = true;
	else if (propname == "positional")
	    positional = true;
	else if (propname == "writable")
	    writable = true;
	else if (propname == "multi")
	    multi = true;
	else if (propname == "spelling")
	    spelling = true;
	else if (propname == "synonyms")
	    synonyms = true;
	else if (propname == "metadata")
	    metadata = true;
	else if (propname == "replicas")
	    replicas = true;
	else if (propname == "valuestats")
	    valuestats = true;
	else if (propname == "inmemory")
	    inmemory = true;
	else if (propname == "flint")
	    flint = true;
	else if (propname == "chert")
	    chert = true;
	else
	    throw Xapian::InvalidArgumentError("Unknown property '" + propname + "' found in proplist");

	if (comma == string::npos)
	    break;
	pos = comma + 1;
    }
}

bool
TestRunner::use_backend(const string & backend_name)
{
    if (user_backend.empty())
	return true;
    if (backend_name == user_backend)
	return true;
    if (startswith(backend_name, user_backend + "_"))
	return true;
    return false;
}

void
TestRunner::set_properties_for_backend(const string & backend_name)
{
    const char * propstring = NULL;
    for (const BackendProperties * i = backend_properties; i->name; ++i) {
	if (backend_name == i->name) {
	    propstring = i->properties;
	    break;
	}
    }
    if (!propstring)
	throw Xapian::InvalidArgumentError("Unknown backend " + backend_name);
    set_properties(propstring);
}

void
TestRunner::do_tests_for_backend(BackendManager * manager)
{
    string backend_name = manager->get_dbtype();
    if (use_backend(backend_name)) {
	backendmanager = manager;
	backendmanager->set_datadir(srcdir + "/testdata/");
	set_properties_for_backend(backend_name);
	cout << "Running tests with backend \"" << backendmanager->get_dbtype() << "\"..." << endl;
	result_so_far = max(result_so_far, run());
    }
    delete manager;
}

int
TestRunner::run_tests(int argc, char ** argv)
{
    result_so_far = 0;
    try {
	test_driver::add_command_line_option("backend", 'b', &user_backend);
	test_driver::parse_command_line(argc, argv);
	srcdir = test_driver::get_srcdir();

	do_tests_for_backend(new BackendManager);

#ifdef XAPIAN_HAS_INMEMORY_BACKEND
	do_tests_for_backend(new BackendManagerInMemory);
#endif

#ifdef XAPIAN_HAS_CHERT_BACKEND
	do_tests_for_backend(new BackendManagerChert);
#endif

#ifdef XAPIAN_HAS_FLINT_BACKEND
	do_tests_for_backend(new BackendManagerFlint);
#endif

#ifdef XAPIAN_HAS_CHERT_BACKEND
	do_tests_for_backend(new BackendManagerMulti("chert"));
#endif
#ifdef XAPIAN_HAS_FLINT_BACKEND
	do_tests_for_backend(new BackendManagerMulti("flint"));
#endif

#ifdef XAPIAN_HAS_REMOTE_BACKEND
#ifdef XAPIAN_HAS_CHERT_BACKEND
	do_tests_for_backend(new BackendManagerRemoteProg("chert"));
	do_tests_for_backend(new BackendManagerRemoteTcp("chert"));
#endif
#ifdef XAPIAN_HAS_FLINT_BACKEND
	do_tests_for_backend(new BackendManagerRemoteProg("flint"));
	do_tests_for_backend(new BackendManagerRemoteTcp("flint"));
#endif
#endif
    } catch (const Xapian::Error &e) {
	cerr << "\nTest harness failed with " << e.get_description() << endl;
	return false;
    } catch (const std::string &e) {
	cerr << "\nTest harness failed with \"" << e << "\"" << endl;
	return false;
    }
    return result_so_far;
}
