<?php
//
// General purpose shell utilities.
//
//------------------------------------------------------------------------------

function git_shell_exec($path, $commands, $abort = TRUE) {
	
	$pwd = getcwd();
  if (!$path || ($path != '.' && !chdir($path))) {
    return FALSE;
  }
  
  if (!is_array($commands)) {
    $commands = array($commands);
  }  
  $status = TRUE;
  
  foreach ($commands as $cmd) {
    $status = drush_shell_exec($cmd, 2);
    
    if ($abort && !$status) {
    	break;
    }   
  }
  ($path != '.' && chdir($pwd)); 
  return $status;
}

//------------------------------------------------------------------------------

function git_shell_capture($path, $commands, $abort = TRUE) {

  $pwd = getcwd();
  if (!$path || ($path != '.' && !chdir($path))) {
    return NULL;
  }
  
  if (!is_array($commands)) {
    $commands = array($commands);
  }  
  $last_command = array_pop($commands);
  
  if ($last_command) {
    $pre_commands = $commands;
    $lines        = NULL;
    
    $status = TRUE;
  
    foreach ($pre_commands as $cmd) {
      $status = drush_shell_exec($cmd, 2);
    
      if ($abort && !$status) {
        break;
      }   
    }    
    if ($status) {
      $lines = array_filter(explode("\n", shell_exec($last_command)));
    }
  }
  ($path != '.' && chdir($pwd));
  return $lines;
}

//------------------------------------------------------------------------------