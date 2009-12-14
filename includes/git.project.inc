<?php
// $Id$

git_include('gitcore', 'xmlutils');

class Project
{
  //****************************************************************************
  // Constants
  //****************************************************************************

	/**
	 * Project configuration file.
	 */
	const CONFIG_FILE = '.gitdrush';
	
	/**
	 * Project identifiers (for constructor)
	 */
	const ID_PATH    = 'path';
	const ID_PROJECT = 'project';
	
  /**
   * Release listing modes.
   * 
   * We can either return all, without dev, or with only the dev releases for
   * a particluar project.
   */
  const MODE_ALL      = 'all';
  const MODE_NO_DEV   = 'nodev';
  const MODE_ONLY_DEV = 'onlydev';
  	
	//****************************************************************************
  // Properties
  //****************************************************************************
  
	protected $name;
	protected $type;
	protected $mode;
	
	protected $initialized = FALSE;
	protected $updated     = FALSE;
	   
  protected $destination;
  protected $path;
  
  protected $suppress_path_init = FALSE;
  	
	protected $drupal_version;
	protected $major;
  
	protected $releases;
	protected $version_map;
	
	protected $recommended_release;
	  	
	//****************************************************************************
  // Methods
  //****************************************************************************
  
	//****************************************************************************
	// Initialization
	
	public function __construct($id = NULL, $id_type = self::ID_PROJECT, $mode = NULL) {
		
		$this->set_drupal_version(self::current_drupal_version());
		
		switch ($id_type) {			
			case self::ID_PROJECT:
				// Look project up on Drupal.org.
				$this->init_by_name($id, $mode);
				break;
				
			case self::ID_PATH:
				// Find project in the filesystem.  It MUST exist as a drush git project!!!!
				$this->init_by_path($id);
				break;
		}
	}
	
	//----------------------------------------------------------------------------
	
	protected function init_by_name($project, $mode = NULL) {
    // These settings are needed to query the project information from 
    // Drupal.org.
    $this->set_name($project);    
    
    // Get project information.
    if (!$this->load_info()) {
      throw new Exception('Retrieval of project information from Drupal.org has failed.');
    }
    // Save initial configuration settings to .gitdrush.  It is safe to set 
    // them after the path is set through load_info().
    $this->set_name($project); // This is not useless.  I swear!
    $this->set_mode($mode);	
	}
	
	//----------------------------------------------------------------------------
	
	protected function init_by_path($path) {
		
		// Set the path so that we can access the repository.
		$this->set_path($path);
		
		// Load various configuration settings needed from the repository.
		$this->get_name();
		$this->get_mode();
	
    // Get project information.
    if (!$this->load_info()) {
      throw new Exception('Retrieval of project information from Drupal.org has failed.');
    }
	}
	
  //****************************************************************************
  // Accessor / Modifiers
  
	public function is_repository() {
		return GitCore::is_repository($this->get_path());
	}
	
	//----------------------------------------------------------------------------
	
	public function is_initialized() {
		return $this->initialized;
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_initialized($bool) {
		$this->initialized = $bool;
	}
	
  //----------------------------------------------------------------------------
  
  public function is_updated() {
    return $this->updated;
  }
  
  //----------------------------------------------------------------------------
  
  protected function set_updated($bool) {
    $this->updated = $bool;
  }
  
	//----------------------------------------------------------------------------
	
	public function get_name() {
		
		if (!$this->name && $this->get_path()) {
		  $this->name = $this->get_config('name', '');	
		}
		return $this->name;
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_name($name) {
		$this->name = $name;
		
		if ($this->get_path()) {
		  $this->set_config('name', $name);
		}
	}
	
	//----------------------------------------------------------------------------
	
	public function get_type() {
		return $this->type;
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_type($type) {
		$this->type = $type;
	}
	
	//----------------------------------------------------------------------------
	
	public function get_mode() {
		
    if (!$this->mode && $this->get_path()) {
      $this->mode = $this->get_config('mode', FALSE);

      if (!$this->mode) {
      	$this->set_mode(self::MODE_NO_DEV);
      }
    }
    return $this->mode;
	}
	
	//----------------------------------------------------------------------------
	
	public function set_mode($mode) {
		
		if ($mode || $this->mode) {
		  switch ($mode) {     
      
        case self::MODE_ALL:
        case self::MODE_NO_DEV:
        case self::MODE_ONLY_DEV:
          break;
      
        default:
          $mode = self::MODE_NO_DEV;
      }
      $this->mode = $mode;
    
      if ($this->get_path()) {
        $this->set_config('mode', $mode);
      }
		}
	}
	
	//----------------------------------------------------------------------------
	
	public function get_destination() {
		return $this->destination;
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_destination($destination) {
		$this->destination = $destination;
	
    if (!$this->is_suppress_path_init() && !is_dir($destination)) {
      mkdir($destination, 0755, TRUE);
    }
	}
	
	//----------------------------------------------------------------------------
	
	public function get_path() {
		return $this->path;
	}
	
	//----------------------------------------------------------------------------
	
	public function set_path($path) {
		$this->path = $path;
		
		if (!$this->is_suppress_path_init()) {
		  if (!is_dir($path)) {
			 mkdir($path, 0755, TRUE);
		  }
		  // Get the repository ready if it isnt already.
		  GitCore::init_repo($path);
		}
	}
	
	//----------------------------------------------------------------------------
	
	public function is_suppress_path_init() {
		return $this->suppress_path_init;
	}
	
	//----------------------------------------------------------------------------
	
	public function set_suppress_path_init($bool) {
		$this->suppress_path_init = $bool;
	}
	
	//----------------------------------------------------------------------------
	
	public function get_drupal_version() {
		return $this->drupal_version;
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_drupal_version($version) {
		$this->drupal_version = $version;
	}
	
	//----------------------------------------------------------------------------
	
	public function get_major() {
		return $this->major;
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_major($major) {
		$this->major = $major;
	}
	
	//----------------------------------------------------------------------------
	
	public function get_releases($versions_only = TRUE) {
		$selection = array();
		
		// TODO: Work on efficiency here!!
	
		if (is_array($this->releases)) {
			$mode = $this->get_mode();
			
      foreach ($this->releases as $major => $releases) {
        usort($releases, array('Project', 'compare_releases'));
      
        foreach ($releases as $release) {
          $is_dev = $release['version_extra'] == 'dev';
        
          if (($is_dev && $mode == self::MODE_NO_DEV)
              || (!$is_dev && $mode == self::MODE_ONLY_DEV)) {
            continue; 
          }

          if ($versions_only) {
            $selection[] = $release['version'];
          }
          else {
        	 $selection[] = $release;   
          }
        }
      }
		}
    return $selection;
	}	
	
  //----------------------------------------------------------------------------
  
  public function get_release($version = NULL) {
    
    if (!$version) {
      return $this->get_recommended_release(FALSE);
    }
    return $this->version_map[$version];
  }
  
	//----------------------------------------------------------------------------
	
	public function get_recommended_release($version_only = TRUE) {
		
		if ($version_only && is_array($this->recommended_release)) {
		  return $this->recommended_release['version'];	
		}
		return $this->recommended_release;
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_recommended_release($release) {
		$this->recommended_release = $release;
	}
		
	//----------------------------------------------------------------------------
	
	public function get_new_releases($time = NULL, $version_only = TRUE) {
	  
		$new_releases = array();
	  $time         = ($time ? $time : $this->get_last_update());
	  
	  foreach ($this->get_releases(FALSE) as $release) {
	  	if ($release['date'] > $time) {
	    	
	  		if ($version_only) {
	    	  $new_releases[] = $release['version'];	
	    	}
	    	else {
          $new_releases[] = $release;
	    	} 
	    }
	  }
	  return $new_releases; 
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_releases($releases) {
		ksort($releases);
		$this->releases = $releases;
	}
	
	//----------------------------------------------------------------------------
	
	/**
	 * TODO: This might be overkill.
	 */
	protected function set_version_map($version_map) {
		$this->version_map = $version_map;
	}
	
	//----------------------------------------------------------------------------
	
	public function get_last_version() {
		return $this->get_config('version', NULL);
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_last_version($version) {
		$this->set_config('version', $version);
	}
	
	//----------------------------------------------------------------------------
	
	public function get_last_update() {
		return $this->get_config('updated', 0);
	}
	
	//----------------------------------------------------------------------------
	
	protected function set_last_update($time) {
		$this->set_config('updated', $time);
	}
	
  //****************************************************************************
  // Utility methods
  
  //----------------------------------------------------------------------------
  
  public function chdir() {
    
  	$path = $this->get_path();
    $pwd  = getcwd();
    
    if ($path && chdir($path)) {
      return $pwd;
    }
    return FALSE;
  }
	
  //----------------------------------------------------------------------------
  
	public function update($version = NULL, $time = NULL, $refresh = FALSE) {
		
		if ((!$this->is_initialized() || $refresh) 
		    && !$this->load_info(TRUE)) {
		  
			return FALSE;
		}
		return (
      $this->download($time)
      && $this->checkout($version)
    );	
	}
	
	//----------------------------------------------------------------------------
	
	public function load_info($refresh = FALSE) {
		
		if (!$refresh && $this->is_initialized()) {
			return TRUE;
		}
		$name           = $this->get_name();
		$drupal_version = $this->get_drupal_version();
				
	  $project_types = pm_project_types();
	  $project_types_xpath = '(value="' . implode('" or value="', $project_types) . '")';	  
	  
	  $url = "http://updates.drupal.org/release-history/$name/$drupal_version";
    
	  if ($xml = @simplexml_load_file($url)) {
	    if ($error = $xml->xpath('/error')) {
	      drush_set_error('DRUSH_PM_COULD_NOT_LOAD_UPDATE_FILE', $error[0]);
	    }
	    else {
	      $recommended_major = $xml->xpath("/project/recommended_major");
	      $recommended_major = (string)$recommended_major[0];   
	        
	      $xml_releases = $xml->xpath("/project/releases/release[status='published']");
	        
	      $releases = $version_map = array();
	      foreach ($xml_releases as $release) {
	        $release = XMLUtils::to_array($release);
	          
	        $releases[$release['version_major']][] = $release;
	        $version_map[$release['version']]      = $release;
	      }                
	
	      // Determine what type of project we have, so we know where to put it.
	      $project_type = 'module';
	
	      if ($types = $xml->xpath('/project/terms/term[name="Projects" and ' . $project_types_xpath . ']')) {
	        $project_type = array_search($types[0]->value, $project_types);
	      }
	        
	      if ($project_type == 'core') {
	        $destination = drush_locate_root();
	          
	        if ($destination === FALSE) {
	          $destination = drush_get_option('destination', '.');
	        }
	        $path = $destination;
	      }
	      else {
	      	// Do not allow setting of the contributed directory on the platform.
	      	drush_set_option('destination', NULL);
	      	
	        $destination = pm_dl_destination($project_type);
	        $path        = $destination . $name;
	      }
	
	      // Main project information properties.
	      $this->set_type($project_type);
	      
	      $this->set_destination($destination ? $destination : '.');
	      $this->set_path($path ? $path : '.');
	      
	      $this->set_major($recommended_major);
	      $this->set_releases($releases);
	      $this->set_version_map($version_map);
	      $this->set_recommended_release($releases[$recommended_major][0]);
	      
	      $this->set_initialized(TRUE);	       
	      return TRUE;
	    }
	  }
	  else {
	    // We are not getting here since drupal.org always serves an XML response.
	    drush_set_error('DRUSH_PM_DOWNLOAD_FAILED', dt('Could not download project status information from !url', array('!url' => $url)));
	  }	
    return FALSE;
	}
	
	//----------------------------------------------------------------------------
	
	/**
	 * Download project packages from Drupal.
	 */
	public function download($time = NULL) {
		
		$name        = $this->get_name();
		$destination = $this->get_destination();
		
		if (!$this->is_initialized() || !$destination) {
			return FALSE;
		}
		$new_releases = $this->get_new_releases($time, FALSE);
		
		if (count($new_releases)) {
		  drush_include_engine('package_handler', drush_get_option('package-handler', 'wget'));
		}
		
    //$i = 2;
    foreach ($new_releases as $release) {
      
    	if (!$this->_precommit($release)) {
      	return FALSE;
      }
      $success = FALSE;
      if (package_handler_install_project($name, $release, $destination)) {
      
        drush_log(dt("Project !project (!version) downloaded to !dest.",
          array(
            '!project' => $name, 
            '!version' => $release['version'], 
            '!dest' => $destination)), 'success'
        );          
        drush_command_invoke_all('drush_git_post_install', $name, $release, $destination);
        $success = TRUE;
      }
      
      if (!$this->_commit($release, $success)) {
        return FALSE;
      } 
      $this->set_last_update($release['date']);
      $this->set_last_version($release['version']);
      
      /*if (! --$i) {
        break;
      }*/
    }
    $this->set_updated(TRUE);
    return TRUE;
	}
	 
  //----------------------------------------------------------------------------
  
  public function clean() {
  
    if (!$this->is_repository()) {
      return FALSE;
    }    
    // Clean directory.
    $path = $this->get_path();
    
    foreach (GitCore::get_repository_files($path) as $file) {
    	
      if ($file == self::CONFIG_FILE) {
        continue;
      } 
      $parts = explode('/', $file);
      
      $file_name = array_pop($parts);
      $file_path = implode('/', $parts);
           
      if ($file_path) {
        if (!unlink("$path/$file_path/$file_name")) {
          return FALSE;
        }
        
        // Gobble up empty directories.
        @rmdir("$path/$file_path");
        for ($index = 0; $index < count($parts); $index++) {
          $file_path = preg_replace('/\/?[^\/]+$/', '', $file_path);
          @rmdir("$path/$file_path");
        }     
      }
      elseif ($file_name) {
        if (!unlink("$path/$file_name")) {
          return FALSE;     
        }
      }     
    }
    return TRUE;
  }
	  
	//----------------------------------------------------------------------------
	
	public function checkout($version = NULL) {
	  
	  if (!$this->is_repository()) {
	  	return FALSE;
	  }	  
	  	  
	  if (!$version) {
	  	$recommended = $this->get_recommended_release(FALSE);
	  	
	  	$drupal_version = $this->get_drupal_version();
	  	$version        = $drupal_version . '-' . $recommended['version_major'];
	  }	  
	  $path     = $this->get_path();	  
	  $branches = array_fill_keys(GitCore::get_branches($path, TRUE), TRUE);
	  
	  if ($this->get_release($version)) {
	  	if(GitCore::checkout_tag($path, $version)) {
        return $this->_postprocess_checkout($version, FALSE);
      }	  	
	  }	
    elseif ($branches[$version]) {
    	if (GitCore::checkout_branch($path, $version)) {
        return $this->_postprocess_checkout($version, TRUE);
      } 
    }
    return FALSE;
	}
	
	//----------------------------------------------------------------------------
	
	protected function _postprocess_checkout($version, $is_branch) {
    // Override me in subclasses!!!
    return TRUE;	
	}
	  
	//----------------------------------------------------------------------------
	
	protected function _precommit($release) {
	  $path = $this->get_path();
	  
	  if (!count(GitCore::get_commits($path, GitCore::COMMIT_ALL, TRUE))) {
	    return TRUE;
	  }	  
	  $drupal_version = $this->get_drupal_version();
	  
	  $branch_name    = $drupal_version . '-' . $release['version_major'];
	  $tag_name       = $release['version'];
	  
	  GitCore::copy_hooks($path);
	  
	  if (!GitCore::checkout_branch($path, $branch_name)) {
	  	return FALSE;
	  }	  
	  // Perform backup commit. ( so we can revert if the download fails )
	  if (!count(GitCore::get_altered_files($path)) 
	      || GitCore::exec($path, array( 
	      'git add .', 
	      'git add -u', 
	      "git commit -m 'Pre-release backup for $tag_name'", 
	    ), TRUE)) {
	    
	    return $this->clean();   
	  }
	  return FALSE;
	}
		
	//----------------------------------------------------------------------------
	
	protected function _commit($release, $success) {
	  
		$name    = $this->get_name();
	  $path    = $this->get_path();
	  $is_new  = FALSE;
	  
	  if (!count(GitCore::get_commits($path))) {
	    if ($success) {   
	      $is_new = TRUE;      
	    }
	    else {
	      return FALSE;
	    }
	  }
	  
	  if ($success) {
	    // For special cases, such as "core".  Blah.
	    if (!$this->_preprocess_commit($release, $is_new)) {
	      return FALSE;
	    }
	    $version = $release['version'];
	    
	    // Perform release commit. ( this one is tagged if not dev )
	    $commands = array(
        'git add .',
        'git add -u',
        "git commit -m '$name release ( $version )'",
	    );
	    if ($release['version_extra'] != 'dev') {
	    	$commands[] = "git tag '$version'";
	    }	    
	    return GitCore::exec($path, $commands, TRUE);     
	  }
	  elseif (!$is_new) {
	    // There were problems.  Roll back our changes.
	    GitCore::exec($path, 'git reset --hard');
	  }
	  return FALSE; 
	}
	
	//----------------------------------------------------------------------------
	
	protected function _preprocess_commit($release, $is_new) {
		// Override me in subclasses!!!
		return TRUE;
	}
	
	//----------------------------------------------------------------------------
	
	static function current_drupal_version() {
		
		if (drush_locate_root()) {
		  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_ROOT);
		}
		return drush_get_context('DRUSH_DRUPAL_MAJOR_VERSION', drush_get_option('version', 6)) . '.x';
	}
	
  //----------------------------------------------------------------------------

  static function compare_releases($release1, $release2) {
    if ($release1['date'] != $release2['date']) {
      return ($release1['date'] < $release2['date'] ? -1 : 1);
    }
    return 0;
  }
  
  //----------------------------------------------------------------------------
  // Internal helpers
  //----------------------------------------------------------------------------
  
  protected function get_config($property, $value = NULL) {
  	  	
  	if (($path = $this->get_path()) && is_file("$path/". self::CONFIG_FILE)) {
  		$value = GitCore::get_file_config($path, self::CONFIG_FILE, "drush.$property", $value);
    }
  	return $value;
  }  
  
  //----------------------------------------------------------------------------
  
  protected function set_config($property, $value) {
  	if (($path = $this->get_path())
  	    && GitCore::set_file_config($path, self::CONFIG_FILE, "drush.$property", $value)) {

  	  // Keep a record of our changes over time, separate from the project
  	  // commits.
  	  if (GitCore::exec($path, array(
              'git add ' . self::CONFIG_FILE,
              "git commit -m 'Project property $property changed to: $value'",
            ), TRUE, TRUE)) {
      	
      	return TRUE;
      }
      return FALSE; 
  	}
  }  
  
  //----------------------------------------------------------------------------
}