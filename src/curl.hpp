/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef FERAL_LIB_CURL_HPP
#define FERAL_LIB_CURL_HPP

#include <curl/curl.h>

#include <feral/VM/VM.hpp>

// for passing to various callbacks
struct curl_vm_data_t
{
	vm_state_t * vm;
	size_t src_id;
	size_t idx;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////// CURL class //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class var_curl_t : public var_base_t
{
	CURL * m_val;
	bool m_owner;
public:
	var_curl_t( CURL * const val, const size_t & src_id, const size_t & idx, const bool owner = true );
	~var_curl_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	CURL * const get();
};
#define CURL( x ) static_cast< var_curl_t * >( x )

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// CURL functions ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern var_fn_t * progress_callback;

int curl_progress_func( void * ptr, curl_off_t to_download, curl_off_t downloaded, 
			curl_off_t to_upload, curl_off_t uploaded );

var_base_t * feral_curl_easy_set_opt_native( vm_state_t & vm, const fn_data_t & fd );

// set some default values
var_base_t * feral_curl_set_default_progress_func( vm_state_t & vm, const fn_data_t & fd );
var_base_t * feral_curl_set_default_progress_func_tick( vm_state_t & vm, const fn_data_t & fd );

#endif // FERAL_LIB_CURL_HPP