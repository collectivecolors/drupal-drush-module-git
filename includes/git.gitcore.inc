<?php
//
// Git core command utilities. (mainly PHP wrappers)
//
//------------------------------------------------------------------------------
class GitCore
{
  //----------------------------------------------------------------------------
  // Constants
  //----------------------------------------------------------------------------
	
	/**
	 * Git branch listing modes.
	 * 
	 * We can either return the current branch, all branches ( as an array ), or
	 * all branches and the current branch in a hash.
	 */
	const BRANCH_ALL      = 'all';
	const BRANCH_CURRENT  = 'current';
	const BRANCH_COMBINED = 'combined';
	
	//----------------------------------------------------------------------------
	
	/**
	 * Git commit listing branches.
	 */
	const COMMIT_ALL    = 'all';
	const COMMIT_MASTER = 'master';
	
	//----------------------------------------------------------------------------
	// Utilities
	//----------------------------------------------------------------------------
	
	static function is_repository($path) {
	  // TODO: Make the Git check more thorough?
	  return ($path && is_dir("$path/.git"));
	}
	
	//----------------------------------------------------------------------------
	
	static function current_branch($path, $clear = FALSE) {
    return GitCore::_get_branches($path, self::BRANCH_CURRENT, $clear);	
	}
	
	//----------------------------------------------------------------------------
	
	static function get_branches($path, $clear = FALSE) {
		return GitCore::_get_branches($path, self::BRANCH_ALL, $clear);
	}
	
	//----------------------------------------------------------------------------
	
	private static function _get_branches($path, $mode = self::BRANCH_ALL, $clear = FALSE) {
	  static $branches = array();
	  
	  if ($clear) {
	    $branches = array();
	  }
	  
	  if (!GitCore::is_repository($path)) {
	    return array();
	  }
	  
	  if (!array_key_exists($path, $branches)) {
	  	$list = GitCore::capture($path, 'git branch');
	      
	    for ($index = 0; $index < count($list); $index++) {
	      if ($list[$index] && preg_match('/^\*\s*/', $list[$index])) {
	        $list[$index] = preg_replace('/^\s*\*\s*/', '', $list[$index]);
	        
	        $branches[$path]['current'] = $list[$index];
	      }
	      else {
	        $list[$index] = trim($list[$index]);
	      }
	    }
	    $branches[$path]['branches'] = $list;
	  }
	  
	  if ($mode == self::BRANCH_CURRENT) {
	    return $branches[$path]['current'];
	  }
	  elseif ($mode == self::BRANCH_COMBINED) { 
	    return $branches[$path];
	  }
	  
	  return $branches[$path]['branches'];
	}
	
	//----------------------------------------------------------------------------
	
	static function checkout_branch($path, $branch, $original = 'master') {
	    
	  if (!GitCore::is_repository($path)) {
	    return FALSE;
	  }  
	  $branches = array_fill_keys(GitCore::get_branches($path, TRUE), TRUE);
	  
	  if ($branches[$branch]) {
	    return GitCore::exec($path, "git checkout '$branch'");
	  }
	  return GitCore::exec($path, "git checkout -b '$branch' '$original'");
	}
	
	//----------------------------------------------------------------------------
	
	/**
	 * TODO: Return more than the SHA-1 hash.  Maybe throw in the commit message,
	 * author, date, etc...  Something damnit!!
	 */
	static function get_commits($path, $branch = self::COMMIT_ALL, $clear = FALSE) {
		static $commits = array();
		
		if ($clear) {
			$commits = array();
		}
		
		if (!GitCore::is_repository($path)) {
			return array();
		}
		if (!is_array($commits[$path][$branch])) {
			
			if (!count(GitCore::capture($path, "git show-ref --heads"))) {
				$commits[$path][$branch] = array();
			}
			else {
        if ($branch == self::COMMIT_ALL) {
          $placeholder = '--all';
        }
        else {
          $placeholder = $branch;
        }
        $commits[$path][$branch] = GitCore::capture($path, "git rev-list $placeholder");
			}
		}
		return $commits[$path][$branch];
	}
	
	//----------------------------------------------------------------------------
	
	static function get_tags($path, $clear = FALSE) {
	  static $tags = array();
	  
	  if ($clear) {
	    $tags = array();
	  }
	    
    if (!GitCore::is_repository($path)) {
      return array();
    }  
	  
	  if (!is_array($tags[$path])) {
	    $tags[$path] = GitCore::capture($path, 'git tag');
	  }	  
	  return $tags[$path];
	}
	
	//----------------------------------------------------------------------------
	
	static function checkout_tag($path, $tag) {
	    
	  if (!GitCore::is_repository($path)) {
      return FALSE;
    } 
	  $tags = array_fill_keys(GitCore::get_tags($path, TRUE), TRUE);
	  
	  if ($tags[$tag]) {
	    return GitCore::exec($path, "git checkout '$tag'");
	  }
	  return FALSE;
	}
	
	//----------------------------------------------------------------------------
		
	/**
	 * Return all hooks in a specified directory. 
	 * 
	 * This function assumes all files in the specified path are Git hook files.
	 */
	static function get_hooks($path, $executable_only = FALSE) {
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
	
	//----------------------------------------------------------------------------
	
	/**
	 * Copy all hooks defined in specified path to specified path.
	 */
	static function copy_hooks($path, $hook_path = NULL) {
	
	  if ($path) {
	    if (!$hook_path) {
	      $hook_path = drush_get_option('hook-dir', $_SERVER['HOME'] . '/hooks');
	    }
	    $hooks = GitCore::get_hooks($hook_path, FALSE);
	  
	    // Add hooks to specified project.
	    foreach ($hooks as $file) {
	      copy("$hook_path/$file", "$path/.git/hooks/$file");
	      chmod("$path/.git/hooks/$file", 0755);
	    }
	  } 
	}

	//----------------------------------------------------------------------------
   
  static function exec($path, $commands, $abort = TRUE, $ensure_repo = FALSE) {
  	
  	if ($ensure_repo && !GitCore::init_repo($path)) {
      return FALSE;
    }
    
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
  
  //----------------------------------------------------------------------------
  
  static function capture($path, $commands, $abort = TRUE, $ensure_repo = FALSE) {
  
    if ($ensure_repo && !GitCore::init_repo($path)) {
      return FALSE;
    }
    
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
  
  //----------------------------------------------------------------------------
  
  static function init_repo($path) {
  	
  	if (GitCore::is_repository($path)) {
  		return TRUE;
  	}
  	if (GitCore::exec($path, 'git init', TRUE, FALSE)) {
  		GitCore::copy_hooks($path);
  		return TRUE;
  	}
  	return FALSE;
  }
  
  //----------------------------------------------------------------------------
  
  static function get_all_configs($path, $limit_to_drush = TRUE) {
  	$configurations = GitCore::capture($path, 'git config -l', TRUE, TRUE);
  	  	
  	if (is_array($configurations)) {
  		if (!$limit_to_drush) {
  		  return $configurations;	
  		}
  		$drush_configs = array();
  		
      foreach ($configurations as $config) {
        $matches = array();
            
        if (preg_match('/^drush\./', $config)) {
          $drush_configs[] = $config;         
        }
      }
      return $drush_configs;      
  	}
  	return array();
  }
  
  //----------------------------------------------------------------------------
  
  static function get_all_file_configs($path, $file, $limit_to_drush = TRUE) {
    $configurations = GitCore::capture($path, "git config -f '$file' -l", TRUE, TRUE);
        
    if (is_array($configurations)) {
      if (!$limit_to_drush) {
        return $configurations; 
      }
      $drush_configs = array();
      
      foreach ($configurations as $config) {
        $matches = array();
            
        if (preg_match('/^drush\./', $config)) {
          $drush_configs[] = $config;         
        }
      }
      return $drush_configs;      
    }
    return array();
  }
  
  //----------------------------------------------------------------------------
  
  static function get_config($path, $property, $default_value = NULL) {
  	
  	// TODO: Make this call more efficient!! ( through property cache )
  	
  	if ($value = GitCore::capture($path, "git config $property", TRUE, TRUE)) {
  		if (count($value) > 1) {
  			return $value;
  		}
  		return $value[0];
  	}  	
  	return $default_value;
  }
  
  //----------------------------------------------------------------------------
  
  static function get_file_config($path, $file, $property, $default_value = NULL) {
    
    // TODO: Make this call more efficient!! ( through property cache )
    
    if ($value = GitCore::capture($path, "git config -f '$file' $property", TRUE, TRUE)) {
      if (count($value) > 1) {
        return $value;
      }
      return $value[0];
    }   
    return $default_value;
  }
  
  //----------------------------------------------------------------------------
  
  static function set_config($path, $property, $value) {
    
  	if (is_numeric($value) || is_string($value)) {
      return GitCore::exec($path, "git config $property '$value'", TRUE, TRUE); 
    }
  }
  
  //----------------------------------------------------------------------------
  
  static function set_file_config($path, $file, $property, $value) {
    
    if (is_numeric($value) || is_string($value)) {
      return GitCore::exec($path, "git config -f '$file' $property '$value'", TRUE, TRUE); 
    }
  }
  
  //----------------------------------------------------------------------------
  
  static function remove_config($path, $property) {
    return GitCore::exec($path, "git config --unset-all $property", TRUE, TRUE);	
  }
  
  //----------------------------------------------------------------------------
  
  static function remove_file_config($path, $file, $property) {
    return GitCore::exec($path, "git config -f '$file' --unset-all $property", TRUE, TRUE);  
  }
  
  //----------------------------------------------------------------------------
  
  static function remove_config_section($path, $section) {
  	return GitCore::exec($path, "git config --remove-section $section", TRUE, TRUE);
  }
  
  //----------------------------------------------------------------------------
  
  static function remove_file_config_section($path, $file, $section) {
    return GitCore::exec($path, "git config -f '$file' --remove-section $section", TRUE, TRUE);
  }
  
  //----------------------------------------------------------------------------
  
  static function get_repository_files($path) {
  	return GitCore::capture($path, "git ls-files --exclude-standard --exclude='.gitignore'", TRUE, TRUE);
  }
  
  //----------------------------------------------------------------------------
  
  static function get_altered_files($path) {
  	return GitCore::capture($path, "git ls-files --exclude-standard --modified --deleted --others", TRUE, TRUE);
  }
  	
	//----------------------------------------------------------------------------
}