<?php
//
// General purpose file system utilities.
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Directory utilities
//------------------------------------------------------------------------------

function git_is_repository($path) {
	// TODO: Make the Git check more thorough?
  return ($path && is_dir("$path/.git"));
}

//------------------------------------------------------------------------------

function git_project_path($project = NULL, $require_repo = FALSE) {
  
  if (!$project || $project == 'drupal') {
    $path = drush_locate_root();
  }
  else {
    git_include('project');
    
    $info = git_project_info($project);
    $path = (is_array($info) ? $info['path'] : NULL);
  } 
  
  if (!$require_repo || git_is_repository($path)) {   
    return $path;
  } 
  return NULL;
}

//------------------------------------------------------------------------------

function git_cd_project($project = NULL) {
  
  $path = git_project_path($project);
  $pwd  = getcwd();
  
  if ($path && chdir($path)) {
    return $pwd;
  }
  return FALSE;
}

//------------------------------------------------------------------------------

function git_drupal_path($require_repo = TRUE) {
  return git_project_path(NULL, $require_repo);
}

//------------------------------------------------------------------------------

function git_cd_drupal() {
  return git_cd_project();
}

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