<?php
//
// General purpose file system utilities.
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Directory utilities
//------------------------------------------------------------------------------

function git_path_init($path, $files = array(), $perm = 0755) {
  
  mkdir($path, $perm, TRUE);

  foreach ($files as $filename => $body) {
    git_file_write("$path/$filename", $body, FALSE); 
  }        
}

//------------------------------------------------------------------------------
// File utilities
//------------------------------------------------------------------------------

function git_file_split_path($filepath) {
	return array(dirname($filepath), basename($filepath));
}

//------------------------------------------------------------------------------

function git_file_write($filepath, $body, $initialize = TRUE) {
  _git_file_write($filepath, $body, 'w', $initialize);
}

//------------------------------------------------------------------------------

function git_file_append($filepath, $body, $initialize = TRUE) {
  _git_file_write($filepath, $body, 'a', $initialize);
}

//------------------------------------------------------------------------------

function _git_file_write($filepath, $body, $mode = 'w', $initialize = TRUE) {
	
	if ($initialize) {
		list($path, $filename) = git_file_split_path($filepath);
		git_path_init($path); 	
	}
  
  if (is_array($body)) {
    $body = implode("\n", $body); 
  }
  
  $fh = fopen($filepath, $mode);
  
  fwrite($fh, $body);
  fclose($fh);
}

//------------------------------------------------------------------------------