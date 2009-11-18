<?php
//
// Git hook utilities.
//
//------------------------------------------------------------------------------

/**
 * Return all hooks in a specified directory. 
 * 
 * This function assumes all files in the specified path are Git hook files.
 */
function git_hooks_from_path($path, $executable_only = FALSE) {
  $hooks = array();
  
  $dh = opendir($path);
  while ($file = readdir($dh)) {

    // if $file isn't this directory or its parent, 
    // add it to the hooks array
    if ($file != '.' 
        && $file != '..'
        && (!$executable_only 
            || is_executable("$path/$file"))) {
      
      $hooks[] = $file;
    }
  }

  closedir($dh);  
  return $hooks;
}

//------------------------------------------------------------------------------

/**
 * Return all hooks in a specified project. 
 */
function git_hooks($project = NULL, $executable_only = TRUE) {
	
	git_include('base.project');
  $path = git_project_path($project, TRUE);
  
  if ($path) {
    return git_hooks_from_path("$path/.git/hooks", $executable_only);	
  }	
}

//------------------------------------------------------------------------------

/**
 * Copy all hooks defined in specified path to specified path.
 */
function git_copy_hooks_into_path($path, $hook_path = NULL) {

	if ($path) {
    if (!$hook_path) {
      $hook_path = drush_get_option('hook-dir', $_SERVER['HOME'] . '/hooks');
    }
    $hooks = git_hooks_from_path($hook_path, FALSE);
  
    // Add hooks to specified project.
    foreach ($hooks as $file) {
      copy("$hook_path/$file", "$path/.git/hooks/$file");
      chmod("$path/.git/hooks/$file", 0755);
    }
  }	
}

//------------------------------------------------------------------------------

/**
 * Copy all hooks defined in specified path to specified repository.
 */
function git_copy_hooks($project = NULL, $hook_path = NULL) {
	
	git_include('base.project');
	$path = git_project_path($project);
	
	if ($path) {
		git_copy_hooks_into_path($path, $hook_path);
  }
}

//------------------------------------------------------------------------------