/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the GNU GPL 3.0 license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#include <feral/std/fs_type.hpp>

#include "../../include/curl_base.hpp"

var_base_t * progress_callback = nullptr;

size_t progress_func_interval_tick_max = 10;

int curl_progress_func( void * ptr, curl_off_t to_download, curl_off_t downloaded, 
			curl_off_t to_upload, curl_off_t uploaded )
{
	// ensure that the file to be downloaded is not empty
	// because that would cause a division by zero error later on
	if( to_download <= 0 ) {
		return 0;
	}

	if( !progress_callback ) return 0;

	static size_t interval_tick = 0;
	if( interval_tick < progress_func_interval_tick_max ) {
		++interval_tick;
		return 0;
	}

	interval_tick = 0;

	static var_flt_t var_to_download( 0.0, 0, 0 ), var_downloaded( 0.0, 0, 0 );
	static var_flt_t var_to_upload( 0.0, 0, 0 ), var_uploaded( 0.0, 0, 0 );

	curl_vm_data_t * vmd = ( curl_vm_data_t * )ptr;

	var_to_download.set_src_id_idx( vmd->src_id, vmd->idx );
	var_to_download.get() = to_download;
	var_downloaded.set_src_id_idx( vmd->src_id, vmd->idx );
	var_downloaded.get() = downloaded;
	var_to_upload.set_src_id_idx( vmd->src_id, vmd->idx );
	var_to_upload.get() = to_upload;
	var_uploaded.set_src_id_idx( vmd->src_id, vmd->idx );
	var_uploaded.get() = uploaded;
	if( !progress_callback->call( * vmd->vm, { nullptr, & var_to_download, & var_downloaded, & var_to_upload, & var_uploaded },
				      {}, {}, vmd->src_id, vmd->idx ) ) {
		vmd->vm->fail( vmd->src_id, vmd->idx, "failed to call progress callback, check error above" );
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////// CURL functions ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

var_base_t * feral_curl_easy_set_opt_native( vm_state_t & vm, const fn_data_t & fd )
{
	CURL * curl = CURL( fd.args[ 1 ] )->get();
	if( !fd.args[ 2 ]->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected an integer as parameter for option type, found: %s",
			 vm.type_name( fd.args[ 2 ] ).c_str() );
		return nullptr;
	}
	int opt = INT( fd.args[ 2 ] )->get().get_si();
	var_base_t * arg = fd.args[ 3 ];
	// for updating callbacks without much code repetition
	var_base_t ** callback = nullptr;
	size_t callback_arg_count = 0;

	int res = CURLE_OK;
	// manually handle each of the options and work accordingly
	switch( opt ) {
	case CURLOPT_FOLLOWLOCATION: // fallthrough
	case CURLOPT_NOPROGRESS: {
		if( !arg->istype< var_int_t >() ) {
			vm.fail( fd.src_id, fd.idx, "expected an integer as parameter for this option, found: %s",
				 vm.type_name( arg ).c_str() );
			return nullptr;
		}
		res = curl_easy_setopt( curl, ( CURLoption )opt, INT( arg )->get().get_si() );
		break;
	}
	case CURLOPT_URL: {
		if( !arg->istype< var_str_t >() ) {
			vm.fail( fd.src_id, fd.idx, "expected a string as parameter for this option, found: %s",
				 vm.type_name( arg ).c_str() );
			return nullptr;
		}
		res = curl_easy_setopt( curl, ( CURLoption )opt, STR( arg )->get().c_str() );
		break;
	}
	case CURLOPT_XFERINFOFUNCTION: {
		callback = & progress_callback;
		callback_arg_count = 4;
		if( !arg->istype< var_nil_t >() ) {
			if( * callback ) var_dref( * callback );
			* callback = nullptr;
			break;
		}
		if( !arg->callable() ) {
			vm.fail( fd.src_id, fd.idx, "expected a callable as parameter for this option, found: %s",
				 vm.type_name( arg ).c_str() );
			return nullptr;
		}
		if( FN( arg )->args().size() + FN( arg )->assn_args().size() < callback_arg_count ) {
			vm.fail( fd.src_id, fd.idx, "expected function to have %zu parameters for this option, found: %zu",
				 callback_arg_count, FN( arg )->args().size() + FN( arg )->assn_args().size() );
			return nullptr;
		}
		if( * callback ) var_dref( * callback );
		var_iref( arg );
		* callback = FN( arg );
		break;
	}
	case CURLOPT_WRITEDATA: {
		if( !arg->istype< var_file_t >() ) {
			vm.fail( fd.src_id, fd.idx, "expected a file as parameter for this option, found: %s",
				 vm.type_name( arg ).c_str() );
			return nullptr;
		}
		FILE * file = FILE( arg )->get();
		const std::string & mode = FILE( arg )->mode();
		if( mode.find( 'w' ) == std::string::npos && mode.find( 'a' ) == std::string::npos ) {
			vm.fail( fd.src_id, fd.idx, "file is not writable, opened mode: %s",
				 mode.c_str() );
			return nullptr;
		}
		if( !file ) {
			vm.fail( fd.src_id, fd.idx, "given file is not open",
				 vm.type_name( arg ).c_str() );
			return nullptr;
		}
		res = curl_easy_setopt( curl, ( CURLoption )opt, file );
		break;
	}
	default: {
		vm.fail( fd.src_id, fd.idx, "operation is not yet implemented" );
		return nullptr;
	}
	}
	return make< var_int_t >( res );
}

var_base_t * feral_curl_set_default_progress_func( vm_state_t & vm, const fn_data_t & fd )
{
	var_base_t * arg = fd.args[ 1 ];
	if( !arg->callable() ) {
		vm.fail( fd.src_id, fd.idx, "expected a callable as parameter for setting default progress function, found: %s",
			 vm.type_name( arg ).c_str() );
		return nullptr;
	}
	if( progress_callback ) var_dref( progress_callback );
	var_iref( arg );
	progress_callback = FN( arg );
	return vm.nil;
}

var_base_t * feral_curl_set_default_progress_func_tick( vm_state_t & vm, const fn_data_t & fd )
{
	var_base_t * arg = fd.args[ 1 ];
	if( !arg->istype< var_int_t >() ) {
		vm.fail( fd.src_id, fd.idx, "expected an integer as parameter for setting default progress function tick interval, found: %s",
			 vm.type_name( arg ).c_str() );
		return nullptr;
	}
	progress_func_interval_tick_max = INT( arg )->get().get_ui();
	return vm.nil;
}