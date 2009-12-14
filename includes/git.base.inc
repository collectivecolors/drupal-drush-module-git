<?php
// $Id$

git_include('project', 'gitcore', 'diskutils');

class Base extends Project
{
  //****************************************************************************
  // Constants
  //****************************************************************************
  
  /**
   * Release listing modes.
   * 
   * We can either return all, without dev, or with only the dev releases for
   * a particluar project.
   */
  const MODE_ALL      = parent::MODE_ALL;
  const MODE_NO_DEV   = parent::MODE_NO_DEV;
  const MODE_ONLY_DEV = parent::MODE_ONLY_DEV;
  
	//****************************************************************************
  // Properties
  //****************************************************************************

	protected $sub_projects         = array();
	protected $projects_initialized = FALSE;
	
  //****************************************************************************
  // Constructor
  //****************************************************************************
  
  function __construct($mode = NULL, $update_projects = TRUE) {
    parent::__construct('drupal', parent::ID_PROJECT, $mode);
    
    $this->init_projects(TRUE, $update_projects);
  }  
	
  //****************************************************************************
  // Accessor / Modifiers
  //****************************************************************************
	
	public function get_projects($names_only = TRUE) {	  
	  
		if ($names_only) {
	    return array_keys($this->sub_projects);
	  }
	  return $this->sub_projects; 
	}
	
	//----------------------------------------------------------------------------
	
	public function get_project($project) {
		
		if (array_key_exists($project, $this->sub_projects)) {
	   return $this->sub_projects[$project];
		}
		return NULL;
	}
		
	//----------------------------------------------------------------------------
	
	public function add_project($project, $mode = self::MODE_NO_DEV) {
		
		if ($project == 'drupal') {
			return FALSE;
		}		
		if (!$this->get_project($project)) {
		  
			$instance = new Project($project, parent::ID_PROJECT, $mode);
		  
			if (!$instance->update()) {
				return FALSE;
			}		  
		  $this->sub_projects[$project] = $instance;
		    
		  if (!GitCore::set_config($this->get_path(), 
		    "drush.project.$project.path", 
		    $instance->get_path())) {
		    
		  	return FALSE;
		  }
		  return $instance;
		}
		return TRUE;
	}
	
  //----------------------------------------------------------------------------
  
  function remove_project($project) {
    
    if ($instance = $this->get_project($project)) {
      $path = $instance->get_path();

      if (!GitCore::exec('.', "rm -fR $path")) {
      	return FALSE;
      }
      unset($this->sub_projects[$project]);
      
      if (!GitCore::remove_config_section($this->get_path(), "drush.project.$project")) {
      	return FALSE;
      }
    }
    return TRUE;
  }
  
	//****************************************************************************
	// Utilities
	//****************************************************************************
	
  public function init_projects($refresh = FALSE, $update_projects = TRUE) {
    
    if (!$refresh && $this->projects_initialized) {
      return TRUE;
    }   
    $path         = $this->get_path();
    $project_info = array();
    
    foreach (GitCore::get_all_configs($path, FALSE) as $config) {
      $matches = array();
            
      if (preg_match('/^drush\.project\.([^\.]+)\.([^\=]+)=(.*)$/', $config, $matches)) {
        $project  = $matches[1];
        $property = $matches[2];
        $value    = $matches[3];
           
        $project_info[$project][$property] = $value;          
      }
    }      
    foreach ($project_info as $project => $info) {
      $instance = new Project($project);
      
      if ($update_projects && !$instance->update()) {
        return FALSE;
      }     
      $this->sub_projects[$project] = $instance;
    }
    
    $this->projects_initialized = TRUE;
    return TRUE;
  }
  
  //----------------------------------------------------------------------------
  
  public function update($version = NULL, $time = NULL, $refresh = FALSE) {
    
    if (!parent::update($version, $time, $refresh)) {
    	return FALSE;
    }    	
      
    if (!$this->init_projects(FALSE, FALSE)) {
    	return FALSE;
    }    	
    foreach ($this->get_projects(FALSE) as $project => $instance) {
      if (!$instance->update()) {
        return FALSE;	
      }
    }
    return TRUE;
  }
  
  //----------------------------------------------------------------------------
  
  protected function _preprocess_commit($release, $is_new) {
  
    if ($this->get_type() != 'core') {
      return TRUE;
    }
  	return (
      $this->_move_core($this->get_path() . '/drupal-' . $release['version'])
      && $this->_process_core($release, $is_new)
      && $this->_update_default_site($release)	  
	  );
  }
  
  //----------------------------------------------------------------------------
  
  protected function _move_core($tar_path) {
    $path = $this->get_path();
    
  	return GitCore::exec('.', array(
        "cp -fR $tar_path/* $path",
        "cp -fR $tar_path/.[a-zA-Z0-9]* $path",
        "rm -fR $tar_path",
      ), TRUE);	
  }
  
  //----------------------------------------------------------------------------
  
  protected function _process_core($release, $is_new) {	  
  	$path = $this->get_path();
  	
  	// Initialize site directories.
	  DiskUtils::init_dir("$path/sites/all/modules", array(
	    'README.txt' => "Put all shared modules for sites on this platform here.\n"
	  ));
	  DiskUtils::init_dir("$path/sites/all/themes", array(
	    'README.txt' => "Put all shared themes for sites on this platform here.\n"
	  ));
	  DiskUtils::init_dir("$path/sites/all/libraries", array(
	    'README.txt' => "Put all shared libraries for sites on this platform here.\n"
	  ));
	
	  if ($is_new) {
	    // Set up public ignores.
	    DiskUtils::write_file("$path/.gitignore", array(
	      '# Ignore data directory.',
	      'sites/default'  
	    ), FALSE);
	      
	    // Set up private ignores.
	    DiskUtils::append_file("$path/.git/info/exclude", array(
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
	    if (!GitCore::set_config($path, 'drush.site',  'false')) {
	      return FALSE;
	    }
	  }  
	  return TRUE;
  }
  
  //----------------------------------------------------------------------------
  
  protected function _update_default_site($release) {
	  
	  $path = $this->get_path() . '/sites/default';
	  	  
	  // Initialize site data file directory.
	  DiskUtils::init_dir("$path/files", array(
	    'README.txt' => array(
	      'All shared platform files go here.',
	      '',
	      'Sync the backup of this directory with the database backup schedule.'  
	    ),  
	  ), 0777);
	          
	  // Allow world write persmissions on default settings file.
	  // This is changed back later ( after initialization ).
	  if (is_file("$path/default.settings.php")) { // This file came about in Drupal 6.
      chmod("$path/default.settings.php", 0777);
	  
      // Add a settings file and set world write permissions.
      copy("$path/default.settings.php", "$path/settings.php");
	  }
	  chmod("$path/settings.php", 0777);
	        
	  // Initialize git repository for the data directory.
	  //
	  // We keep this directory separate from the site so we can have different sites
	  // powered by the same platform.  We can always clone this directory to create
	  // new sites though.
	  if (!GitCore::is_repository($path)) {
	    if (!GitCore::init_repo($path)) {
	      return FALSE;
	    }
	  }
	  else {
	  	$branch_name = $this->get_drupal_version() . '-' . $release['version_major'];
	  	
	    if (!GitCore::checkout_branch($path, $branch_name)) {
	      return FALSE;
	    }
	  }	        
	  // Add all files and commit.
	  $version = $release['version'];
  
	  $commands = array(
      'git add .',
      'git add -u',
      "git commit --allow-empty -m 'Default site data directory ( $version )'",
    );
    if ($release['version_extra'] != 'dev') {
      $commands[] = "git tag '$version'";
    }
	  return GitCore::exec($path, $commands, TRUE);	
  }
  
  //----------------------------------------------------------------------------
  
  protected function _postprocess_checkout($version, $is_branch) {
    
  	if ($this->get_type() != 'core') {
  		return TRUE;
  	}
  	$path = $this->get_path() . '/sites/default';
  	  	
  	if ($is_branch) {
  		return GitCore::checkout_branch($path, $version);
  	}
  	return GitCore::checkout_tag($path, $version);
  }
  	
	//----------------------------------------------------------------------------
}