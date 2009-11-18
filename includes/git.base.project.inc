<?php
//
// Git project utilities.
//
//------------------------------------------------------------------------------

/**
 * Release listing modes.
 * 
 * We can either return all, without dev, or with only the dev releases for
 * a particluar project.
 */
define('GIT_LIST_ALL', 'all');
define('GIT_LIST_NO_DEV', 'nodev');
define('GIT_LIST_ONLY_DEV', 'onlydev');

//------------------------------------------------------------------------------

function git_project_path($project = NULL, $require_repo = FALSE) {
  
  if (!$project || $project == 'drupal') {
    $path = drush_locate_root();
  }
  else {
    $info = git_project_info($project);
    $path = (is_array($info) ? $info['path'] : NULL);
  }
  git_include('core');
  
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

function git_load_projects($names_only = TRUE, $clear = FALSE) {
  static $projects = array();
  
  if ($clear) {
  	$projects = array();
  }

  if (!count($projects)) {
    $configurations = git_shell_capture(git_drupal_path(), 'git config -l');

    if (is_array($configurations)) {
      foreach ($configurations as $config) {
        $matches = array();
          
        if (preg_match('/^drush\.project\.([^\.]+)\.([^\=]+)=(.*)$/', $config, $matches)) {
          $project  = $matches[1];
          $property = $matches[2];
          $value    = $matches[3];
          
          $projects[$project][$property] = $value;          
        }
      }
    }
  }  
  if ($names_only) {
    return array_keys($projects);
  }
  return $projects; 
}

//------------------------------------------------------------------------------

function git_project_load($project, $clear = FALSE) {
	static $project_info = NULL;
	
	if ($clear) {
		$project_info = NULL;
	}
	
	if (is_null($project_info)) {
		$project_info = git_load_projects(FALSE, TRUE);
	}
	return $project_info[$project];
}

//------------------------------------------------------------------------------

function git_project_add($project, $mode = GIT_LIST_NO_DEV, $checkout_branch = TRUE) {
	
	if (!_git_project_download($project, $mode, $checkout_branch)) {
		return FALSE;
	}
	$info    = git_project_info($project);
	$release = git_project_release($project);
		
	$configs = array(
	  'path'     => $info['path'],
	  'version'  => $release['version'],
	  'date'     => $release['date'],
    'mode'     => $mode,
    'checkout' => ($checkout_branch ? 'branch' : 'tag'),
	);
			
	$commands = array();		
	foreach ($configs as $property => $value) {
	  $commands[] = "git config drush.project.$project.$property $value";	
	}		
	return git_shell_exec($info['path'], $commands);
}

//------------------------------------------------------------------------------

function git_project_update($project) {
	
	$info = git_project_load($project, TRUE);
	
	if (is_array($info)
	   && _git_project_download($project, $info['mode'], $info['checkout'])) {

    return TRUE;
  }
	return FALSE;	
}

//------------------------------------------------------------------------------

function git_project_remove($project) {
  // TODO: Complete this function!!!!	
}

//------------------------------------------------------------------------------

function git_project_info($projects, $force_array = FALSE) {
  static $project_cache = array();
  
  git_include('xml');
  
  // Don't require an array for the parameters.
  if (!is_array($projects)) {
    $projects = array($projects);
  }
  
  $project_types = pm_project_types();
  $project_types_xpath = '(value="' . implode('" or value="', $project_types) . '")';
  
  foreach (pm_parse_project_version($projects) as $project => $specs) {
    
    if (array_key_exists($project, $project_cache)) {     
      continue; 
    }
    
    // Don't rely on UPDATE_DEFAULT_URL since we are not fully bootstrapped.
    $url = "http://updates.drupal.org/release-history/$project/" 
           . $specs['drupal_version'];

    if ($xml = @simplexml_load_file($url)) {
      if ($error = $xml->xpath('/error')) {
        drush_set_error('DRUSH_PM_COULD_NOT_LOAD_UPDATE_FILE', $error[0]);
      }
      else {
        $recommended_major = $xml->xpath("/project/recommended_major");
        $recommended_major = (string)$recommended_major[0];   
        
        $xml_releases = $xml->xpath("/project/releases/release[status='published']");
        
        $releases = array();
        foreach ($xml_releases as $release) {
          $release = git_xml_to_array($release);
          $releases[$release['version_major']][] = $release;
        }             

        // Determine what type of project we have, so we know where to put it.
        $project_type = 'module';

        if ($types = $xml->xpath('/project/terms/term[name="Projects" and ' . $project_types_xpath . ']')) {
          $project_type = array_search($types[0]->value, $project_types);
        }
        
        if ($project_type == 'core') {
          $destination = drush_locate_root();
          $path        = $destination;
        }
        else {
          $destination = pm_dl_destination($project_type);
          $path        = $destination . $project;
        }

        $project_cache[$project] = array(
          'name'        => $project,
          'drupal'      => $specs['drupal_version'],
          'type'        => $project_type,
          'destination' => ($destination ? $destination : '.'),
          'path'        => ($path ? $path : '.'),
          'major'       => $recommended_major,
          'releases'    => $releases,
          'release'     => $releases[$recommended_major][0],          
        );
      }
    }
    else {
      // We are not getting here since drupal.org always serves an XML response.
      drush_set_error('DRUSH_PM_DOWNLOAD_FAILED', dt('Could not download project status information from !url', array('!url' => $url)));
    }
  }
  
  // Return a single package if only one package name was specified.
  if (!$force_array && count($projects) == 1) {
    return $project_cache[$projects[0]];
  }
  
  // Else return a hash with the packages.
  $project_return = array();
  foreach ($projects as $name) {
    $project_return[$name] = $project_cache[$name];
  }
  return $project_return;
}

//------------------------------------------------------------------------------

/**
 * List public releases of a Drupal project.
 * 
 * Has three modes:
 * 
 * GIT_LIST_ALL       - All releases including dev releases.
 * GIT_LIST_NO_DEV    - All releases except dev releases.
 * GIT_LIST_ONLY_DEV  - Only the dev releases.
 */
function git_project_releases($project, $mode = GIT_LIST_ALL) {
  static $versions = array();
  
  $info = git_project_info($project);
  
  if (!array_key_exists($project, $versions)
     || array_key_exists($mode, $versions[$project])) {     
    
    ksort($info['releases']);
  
    foreach ($info['releases'] as $major => $releases) {
      usort($releases, '_git_release_compare');
      
      foreach ($releases as $release) {
        $is_dev = $release['version_extra'] == 'dev';
        
        if (($is_dev && $mode == GIT_LIST_NO_DEV)
            || (!$is_dev && $mode == GIT_LIST_ONLY_DEV)) {
          continue; 
        }               
        $versions[$project][$mode][] = $release['version'];   
      }
    }
  }  
  return $versions[$project][$mode];  
}

//------------------------------------------------------------------------------

/**
 * Return release information for a specified version of a project.
 * 
 * This function has two usage formats:
 * 
 * $release = git_project_release('admin_menu-6.x-1.5');
 * 
 *  '- Returns the current recommended version if specified version not found.
 * 
 * $release = git_project_release('admin_menu', '6.x-1.5');
 * 
 *  '- Returns NULL if specified version not found.
 */
function git_project_release() {
  
	// TODO: Make this function more efficient with a static cache.
	
  $args     = func_get_args();
  $num_args = count($args);
  
  if (!$num_args) {
    return NULL;
  }
  elseif ($num_args == 1) {
    // Assume the parameter is a package name.  Ex: admin_menu-6.x-1.5
    
    foreach (pm_parse_project_version($args) as $name => $specs) {
      $project     = $name;
      $version     = $specs['version'];
      $use_default = TRUE;
    }   
  }
  else {
    // Assume the parameters are project name and version. admin_menu, 6.x-1.5
    $project = $args[0];
    $version = $args[1];
  }
    
  if ($info = git_project_info($project)) {
    krsort($info['releases']);
    
    foreach ($info['releases'] as $major => $releases) {
      foreach ($releases as $release) {
        if ($release['version'] == $version) {
          return $release;
        }
      }
    }
    
    if ($use_default) {
      return $info['release'];
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------

function git_project_new_releases($project, $mode = GIT_LIST_NO_DEV) {
	$new_releases = array();
	
	git_include('core');
  
  if (git_project_path($project, TRUE)) {
  	
  	$info     = git_project_load($project, TRUE);
  	$releases = git_project_releases($project, $mode);
    
    foreach ($releases as $version) {
    	$release = git_project_release($project, $version);
    	
    	if (!is_array($info) || $release['date'] > $info['date']) {
        $new_releases[] = $release;	
    	}
    }
  } 
  return $new_releases; 
}

//------------------------------------------------------------------------------

function git_project_checkout($project, $version, $checkout_branch = TRUE) {
  
  $info = git_project_info($project);
  
  if (!is_array($info)) {
    return FALSE;
  } 
  git_include('core');
  
  $path = $info['path'];     
  if (!git_is_repository($path)) {
    return FALSE;
  }
  
  $type           = $info['type'];
  $drupal_version = $info['drupal'];
  
  $release = git_project_release($project, $version);
  
  if (!is_array($release)) {
    return FALSE;
  }  
  $major_version  = $release['version_major'];
  
  $branch_name = $drupal_version . '-' . $major_version;
  $tag_name    = $release['version'];
  
  if ($checkout_branch) {
    if (git_checkout_branch($path, $branch_name)) {
      if ($type == 'core') {
        return git_checkout_branch("$path/sites/default", $branch_name);
      }
      return TRUE;
    }
  }
  else {
    if(git_checkout_tag($path, $tag_name)) {
      if ($type == 'core') {
        return git_checkout_tag("$path/sites/default", $tag_name);
      }
      return TRUE;
    }
  }
  return FALSE;
}

//------------------------------------------------------------------------------

function _git_project_clean($path) {

	git_include('core', 'shell');
	
  if (!git_is_repository($path)) {
    return;
  }
  
  // Clean directory.
  $repo_files = git_shell_capture($path, "git ls-files --exclude-standard --exclude='.gitignore'");
  
  foreach ($repo_files as $file) {
    $parts = explode('/', $file);
    
    $file_name = array_pop($parts);
    $file_path = implode('/', $parts);
    
    if ($file_path) {
      unlink("$path/$file_path/$file_name");
      
      @rmdir("$path/$file_path");
      for ($index = 0; $index < count($parts); $index++) {
        $file_path = preg_replace('/\/?[^\/]+$/', '', $file_path);
        @rmdir("$path/$file_path");
      }     
    }
    elseif ($file_name) {
      unlink("$path/$file_name");     
    }     
  }
}

//------------------------------------------------------------------------------

/**
 * Download a project package from Drupal.
 */
function _git_project_download($package, $mode = GIT_LIST_NO_DEV, $checkout_branch = TRUE) {
  drush_include_engine('package_handler', drush_get_option('package-handler', 'wget'));

  if ($info = git_project_info($package)) {
  	$project = $info['name'];
    
    $destination = $info['destination'];
    $my_release  = git_project_release($package);
    
    if ($destination && is_array($my_release)) {
      $i = 1;
      foreach (git_project_new_releases($project, $mode) as $release) {
        
        _git_project_precommit($info, $release);

        $success = FALSE;
        if (package_handler_install_project($project, $release, $destination)) {
        
          drush_log(dt("Project !project (!version) downloaded to !dest.",
            array(
              '!project' => $project, 
              '!version' => $release['version'], 
              '!dest' => $destination)), 'success'
          );          
          drush_command_invoke_all('drush_pm_post_install', $project, $release, $destination);
          $success = TRUE;
        }

        _git_project_commit($info, $release, $success);
        if (! --$i) {
          break;
        }
      }

      if (git_project_checkout($project, $my_release['version'], $checkout_branch)) {
        return TRUE;
      }
    }   
  }
  return FALSE;
}

//------------------------------------------------------------------------------

function _git_project_precommit($info, $release) {
  
  $project = $info['name'];
  $path    = $info['path'];
  
  git_include('core', 'shell', 'hook');
  
  if (!git_is_repository($path)) {
    return;
  }
  
  $drupal_version = $info['drupal'];
  $major_version  = $release['version_major'];
  
  $branch_name    = $drupal_version . '-' . $major_version;
  $tag_name      = $release['version'];
  
  // Copy hooks.
  git_copy_hooks($project);
  
  // Checkout major branch.
  git_checkout_branch($path, $branch_name);
  
  // Perform backup commit. ( so we can revert if the download fails )
  if (git_shell_exec($path, array( 
      'git add .', 
      'git add -u', 
      "git commit -m 'Pre-release backup for $tag_name'", 
    ), TRUE)) {
      
    _git_project_clean($path);   
  }
}

//------------------------------------------------------------------------------

function _git_project_commit($info, $release, $success) {
  
  git_include('core', 'shell');
  
  $project = $info['name'];
  
  $type    = $info['type'];
  $path    = $info['path'];
  
  // Initialize repository.
  $is_new = FALSE;
  
  if (!git_is_repository($path)) {
    if ($success) {   
      if (!git_shell_exec($path, 'git init')) {
        return FALSE;
      }
      $is_new = TRUE;      
    }
    else {
      return FALSE;
    }
  }
  
  if ($success) {
    // Initialize Drupal core before commit.
    if ($type == 'core') {
    	// Special case.  Blah.
      if (!_git_project_update_core($info, $release, $is_new)) {
      	return FALSE;
      }
    }    
    $tag_name = $release['version'];
        
    // Perform release commit. ( this one is tagged )
    return git_shell_exec($path, array(
        'git add .',
        'git add -u',
        "git commit -m '$project release ( $tag_name )'",
        "git tag '$tag_name'",
      ), TRUE);     
  }
  else {
    // Roll back our changes.
    return git_shell_exec($path, 'git reset --hard');    
  }
}

//------------------------------------------------------------------------------

function _git_project_update_core($info, $release, $is_new) {
  
  $path    = $info['path'];
  $version = $release['version'];
  
  git_include('shell');
  
  // Move drupal directory into root directory.
  $tar_path = "$path/drupal-$version";
  
  if (git_shell_exec('.', array(
      "cp -fR $tar_path/* $path",
      "cp -fR $tar_path/.* $path 2> /dev/null",
    ), TRUE)) {
    	
    rmdir($tar_path);

    // Make changes to Drupal repository before committing.
    return (_git_project_process_core($info, $release, $is_new)
         && _git_project_update_default_site($info, $release));
  }
  return FALSE;  
}

//------------------------------------------------------------------------------

function _git_project_process_core($info, $release, $is_new) {
  
  $path = $info['path'];
  
  git_include('shell', 'filesystem');
  
  // Initialize site directories.
  git_path_init("$path/sites/all/modules", array(
    'README.txt' => "Put all shared modules for sites on this platform here.\n"
  ));
  git_path_init("$path/sites/all/themes", array(
    'README.txt' => "Put all shared themes for sites on this platform here.\n"
  ));
  git_path_init("$path/sites/all/libraries", array(
    'README.txt' => "Put all shared libraries for sites on this platform here.\n"
  ));

  if ($is_new) {
    // Set up public ignores.
    git_file_write("$path/.gitignore", array(
      '# Ignore data directory.',
      'sites/default'  
    ), FALSE);
      
    // Set up private ignores.
    git_file_append("$path/.git/info/exclude", array(
      '',
      '# Ignore everything in modules, themes, and library directories except README.txt',
      'sites/all/modules/*',
      '!sites/all/modules/README.txt',
      '',
      'sites/all/themes/*',
      '!sites/all/themes/README.txt',
      '',
      'sites/all/libraries/*',
      '!sites/all/libraries/README.txt'  
    ), FALSE);    
    
    // Mark this as a non base repository for add and update commands.
    if (!git_shell_exec($path, 'git config drush.site false')) {
    	return FALSE;
    }
  }  
  return TRUE;
}

//------------------------------------------------------------------------------

function _git_project_update_default_site($info, $release) {
  
	$path = $info['path'] . '/sites/default';
    
  $drupal_version = $info['drupal'];
  $major_version  = $release['version_major'];
  
  $branch_name    = $drupal_version . '-' . $major_version;
  $tag_name      = $release['version'];
  
  git_include('core', 'shell', 'filesystem');
  
  // Initialize site data file directory.
  git_path_init("$path/files", array(
    'README.txt' => array(
      'All shared platform files go here.',
      '',
      'Sync the backup of this directory with the database backup schedule.'  
    ),  
  ), 0777);
          
  // Allow world write persmissions on default settings file.
  // This is changed back later ( after initialization ).
  chmod("$path/default.settings.php", 0777);
  
  // Add a settings file and set world write permissions.
  copy("$path/default.settings.php", "$path/settings.php");
  chmod("$path/settings.php", 0777);
        
  // Initialize git repository for the data directory.
  //
  // We keep this directory separate from the site so we can have different sites
  // powered by the same platform.  We can always clone this directory to create
  // new sites though.
  if (!_git_is_repository($path)) {
    if (!git_shell_exec($path, 'git init')) {
    	return FALSE;
    }
  }
  else {
    if (!git_checkout_branch($path, $branch_name)) {
    	return FALSE;
    }
  }        
  // Copy all hooks over from ~/hooks (if they exist)
  git_copy_hooks_into_path($path);
        
  // Add all files and commit.
  return git_shell_exec($path, array(
      'git add .',
      'git add -u',
      "git commit -m 'Default site data directory ( $tag_name )'",
      "git tag '$tag_name'"
    ), TRUE);
}

//------------------------------------------------------------------------------

function _git_release_compare($release1, $release2) {
  if ($release1['date'] != $release2['date']) {
    return ($release1['date'] < $release2['date'] ? -1 : 1);
  }
  return 0;
}

//------------------------------------------------------------------------------
