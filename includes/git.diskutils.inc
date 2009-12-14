<?php
//
// General purpose file system utilities.
//
//------------------------------------------------------------------------------
class DiskUtils
{
  //****************************************************************************
	// Directory utilities
	//****************************************************************************
	
	static function init_dir($path, $files = array(), $perm = 0755) {
	  
		if (!is_dir($path)) {
      mkdir($path, $perm, TRUE);
		}	
	  foreach ($files as $filename => $body) {
	    DiskUtils::write_file("$path/$filename", $body, FALSE); 
	  }        
	}
	
	//****************************************************************************
	// File utilities
	//****************************************************************************
	
	static function split_path($filepath) {
		if (is_dir($filepath)) {
			return array($filepath);
		}
		return array(dirname($filepath), basename($filepath));
	}
	
	//----------------------------------------------------------------------------
	
	static function write_file($filepath, $body, $initialize = TRUE) {
	  return DiskUtils::_write_file($filepath, $body, 'w', $initialize);
	}
	
	//----------------------------------------------------------------------------
	
	static function append_file($filepath, $body, $initialize = TRUE) {
	  return DiskUtils::_write_file($filepath, $body, 'a', $initialize);
	}
	
	//----------------------------------------------------------------------------
	
	private static function _write_file($filepath, $body, $mode = 'w', $initialize = TRUE) {
		
		if ($initialize) {
			list($path, $filename) = DiskUtils::split_path($filepath);
			DiskUtils::init_dir($path); 	
		}
	  
	  if (is_array($body)) {
	    $body = implode("\n", $body); 
	  }
	  
	  $fh = fopen($filepath, $mode);
	  
	  fwrite($fh, $body);
	  fclose($fh);
	}
	
	//----------------------------------------------------------------------------
}