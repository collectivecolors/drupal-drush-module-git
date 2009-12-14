<?php
// $Id$

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Drush hooks
//------------------------------------------------------------------------------

/**
 * @file
 *   Git drush commands.
 *
 *   Commands pertaining to managing Drupal instances with Git.
 *
 *   You can copy this files folder to any of the following
 *     1. A .drush folder in your HOME folder.
 *     2. Anywhere in a folder tree below an active module on your site.
 *     3. In an arbitrary folder specified with the --include option.
 */

/**
 * Implementation of hook_drush_command().
 *
 * In this hook, you specify which commands your
 * drush module makes available, what it does and 
 * description.
 *
 * Notice how this structure closely resembles how 
 * you define menu hooks.
 * 
 * @See drush_parse_command() for a list of recognized keys.
 *
 * @return
 *   An associative array describing your command(s).
 */
function git_drush_command() {
  $items = array();

  $items['git base init'] = array(
    'callback' => 'drush_git_base_init',
    'description' => "Initialize a Drupal base repository.",
    'arguments' => array(
      'directory' => 'Directory to put this Drupal site into.  Defaults to "drupal-$VERSION".',
    ),
    'options' => array(
      '--version' => 'A specific version of Drupal to download. Only used when "--base" option is specified.',
      '--mode' => 'The download and commit mode for this project. Allowed values are; "all", "nodev", and "onlydev". Only used when "--base" option is specified.',      
    ),
    'examples' => array(
      'git base init --version=6 drupal' => 'Initialize a Drupal 6 base repository into the "drupal" directory.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['git site init'] = array(
    'callback' => 'drush_git_site_init',
    'description' => "Initialize a new Drupal site.",
    'arguments' => array(
      'origin' => 'Origin url of the base repository for this site.',
      'directory' => 'Directory to put this Drupal site into.  Defaults to current directory.',
    ),
    'options' => array(
      '--data-url' => 'Git url for the Drupal site data repository.',
      '--data-path' => 'Directory to put the site data repository into.',
      '--checkout' => 'Checkout the head of a branch or a particular tag for this site.',      
    ),
    'examples' => array(
      'git site init git://example.com/drupal6' => 'Initialize a new Drupal site into the current directory.',
      'git site init git://example.com/drupal6 mysite' => 'Initialize a new Drupal site into the "mysite" directory.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['git site clone'] = array(
    'callback' => 'drush_git_site_clone',
    'description' => "Clone an existing Drupal site.",
    'arguments' => array(
      'origin' => 'Origin url of the site repository to clone from.',
      'directory' => 'Directory to put this Drupal site into.  Defaults to current directory.',
    ),
    'options' => array(
      '--data-url' => 'Git url for the Drupal site data repository.',
      '--data-path' => 'Directory to put the site data repository into.',
      '--checkout' => 'Checkout the head of a branch or a particular tag for this site.',        
    ),
    'examples' => array(
      'git site clone git://example.com/mysite' => 'Clone the "mysite" Drupal site into the current directory.',
      'git site clone git://example.com/mysite mysite' => 'Initialize a new Drupal site into the "mysite" directory.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  
  $items['git add'] = array(
    'callback' => 'drush_git_add',
    'description' => "Install new Drupal projects in a base repository or site.",
    'arguments' => array(
      'projects' => 'A space separated list of project names.',
    ),
    'options' => array(
      '--mode' => 'The download and commit mode for the projects being added. Allowed values are; "all", "nodev", and "onlydev"',      
    ),
    'examples' => array(
      'git add cck zen' => 'Download latest versions of CCK and Zen for this version of Drupal.',
      'git add --mode=all cck zen' => 'Download latest versions (including dev releases) of CCK and Zen for this version of Drupal.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT,
  );  
  
  $items['git rm'] = array(
    'callback' => 'drush_git_remove',
    'description' => "Remove Drupal projects in a base repository or site.",
    'arguments' => array(
      'projects' => 'A space separated list of project names.',
    ),
    'examples' => array(
      'git rm cck zen' => 'Remove the CCK and Zen projects from this Drupal installation.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT,
  );  
  
  $items['git update'] = array(
    'callback' => 'drush_git_update',
    'description' => "Update Drupal repository and all sub projects.",
    'examples' => array(
      'git update' => 'Update all projects in base Drupal repository or site', 
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT,
  );  

  return $items;
}

//------------------------------------------------------------------------------

/**
 * Implementation of hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help <name-of-your-command>'
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function git_drush_help($section) {
  switch ($section) {
    case 'drush:git base init':
      return dt("Initialize a Drupal base repository.");
      
    case 'drush:git site init':
      return dt("Initialize a new Drupal site.");
      
    case 'drush:git site clone':
      return dt("Clone an existing Drupal site.");
      
    case 'drush:git add':
      return dt("Install new Drupal projects in a base repository or site.");
      
    case 'drush:git rm':
      return dt("Remove Drupal projects in a base repository or site.");
      
    case 'drush:git update':
      return dt("Update Drupal repository and all sub projects.");
  }
}

//------------------------------------------------------------------------------
// Callbacks

/**
 * Initialize a Drupal base repository.
 * 
 * git base init [ $DESTINATION ]
 *    [ --version=$VERSION ] 
 *    [ --mode=$MODE ]
 * 
 * $DESTINATION = Destination directory for the project.
 * 
 * $VERSION  = Drupal version to retrieve. Ex; 6
 * $MODE     = MODE_ALL ('all') | MODE_NO_DEV ('nodev') | MODE_ONLY_DEV ('onlydev')
 */
function drush_git_base_init() {
	git_include('base');
	
	list($directory) = func_get_args();
	if ($directory) {
		drush_set_option('destination', $directory);
	}	
	$drupal = new Base(drush_get_option('mode', Base::MODE_NO_DEV), FALSE);
	return $drupal->update();
}

//------------------------------------------------------------------------------

/**
 * Initialize a new Drupal site.
 * 
 * git site init $BASE_URL [ $DESTINATION ]
 *    [ --data-url=$DATA_URL ] 
 *    [ --data-path=$DATA_PATH ]
 *    [ --checkout=$CHECKOUT ]
 *
 * $BASE_URL = Base repository url to clone from. 
 * $DESTINATION = Destination directory for the project.
 * 
 * $DATA_URL = Drupal data repository url. Defaults to "$BASE_URL/sites/default"
 * $DATA_PATH = Directory to put the site data repository into. Defaults to "sites/default".
 * $CHECKOUT = Git tag or branch to checkout when initialized.
 */
function drush_git_site_init() {
	git_include('site');
	
	list($base_url, $directory) = func_get_args();
	
	if ($directory) {
		drush_set_option('destination', $directory);
	}
	$drupal = new Site($base_url);
	
	if ($data_url = drupal_get_option('data-url')) {
    $drupal->set_data_url($data_url);
	}
	if ($data_path = drupal_get_options('data-path')) {
		$drupal->set_data_path($data_path);
	}
	
	return $drupal->update(drupal_get_option('checkout', NULL));
	
	// Default site information.
	/*$site_url = preg_replace('/\/?$/', '', $base_url) . '/sites/default';
	  
	// Initialize Drupal site.
	drush_shell_exec("git clone '$base_url' '$directory'");	
	  
  if (($directory == '.' || chdir($directory)) && is_dir('.git')) {
  	// Clone the default site directory.
  	drush_shell_exec("git clone '$site_url' 'sites/default'");
  	
  	// Mark this as a non base repository for add and update commands.
    drush_shell_exec("git config drush.site true");
  }*/
}

//------------------------------------------------------------------------------

/**
 * Clone an existing Drupal site.
 * 
 * git site clone $SITE_URL [ $DESTINATION ]
 *    [ --data-url=$DATA_URL ] 
 *    [ --data-path=$DATA_PATH ]
 *    [ --checkout=$CHECKOUT ]
 *
 * $SITE_URL = Site repository url to clone from. 
 * $DESTINATION = Destination directory for the project.
 * 
 * $DATA_URL = Drupal data repository url. Defaults to "$SITE_URL/sites/default"
 * $DATA_PATH = Directory to put the site data repository into. Defaults to "sites/default".
 * $CHECKOUT = Git tag or branch to checkout when initialized.
 */
function drush_git_site_clone() {
	
}

//------------------------------------------------------------------------------

/**
 * git add
 * 
 * drush git add [ --mode=$MODE ] $PROJECT ...
 *               $PROJECT ...
 *
 * Install or update packages in a base drupal directory or a new Drupal site.
 * 
 * If you are creating a base Drupal repository:
 * 
 *  * The packages are downloaded from Drupal through drush.
 * 
 *  * Packages are updated and recommitted if they already exist in the 
 *    directory structure.
 * 
 * If you are creating a new Drupal site:
 * 
 *  * All packages are added as submodules to the core repository from the 
 *    base repo.
 *
 */
function drush_git_add() {
	git_include('gitcore');
	
	$path = drush_locate_root();	
  if (!GitCore::is_repository($path)) {
		return;
	}
		
  $args = func_get_args();
  
  if (GitCore::get_config($path, 'drush.site') == 'false') {
  	_git_add_base($args);
  }
  else {
    //_git_add_site($args);      
  }
}

//------------------------------------------------------------------------------

function _git_add_base($projects) {
	git_include('base');
	
	$drupal = new Base(NULL, FALSE);
	
	foreach ($projects as $name) {
		
		if ($name == 'drupal') {
			continue;
		}		
		if (!$drupal->add_project($name, drush_get_option('mode', Base::MODE_NO_DEV))) {
			return FALSE;
		}
	}
	return TRUE;	
}

//------------------------------------------------------------------------------

function _git_add_site($projects) {
	
}

//------------------------------------------------------------------------------

/**
 * git rm
 * 
 * drush git rm $PROJECT ...
 *
 * Remove installed projects in a base Drupal directory or a new Drupal site.
 */
function drush_git_remove() {
  git_include('gitcore');
  
  $path = drush_locate_root();  
  if (!GitCore::is_repository($path)) {
    return;
  }
    
  $args = func_get_args();
  
  if (GitCore::get_config($path, 'drush.site') == 'false') {
    _git_remove_base($args);
  }
  else {
    //_git_remove_site($args);      
  }
}

//------------------------------------------------------------------------------

function _git_remove_base($projects) {
  git_include('base');
  
  $drupal = new Base(NULL, FALSE);
  
  foreach ($projects as $name) {
    
    if (!$drupal->remove_project($name)) {
    	return FALSE;
    }
  }
  return TRUE;  
}

//------------------------------------------------------------------------------

function _git_remove_site($projects) {
  
}

//------------------------------------------------------------------------------

/**
 * git update
 * 
 * drush git update
 *
 * Update all packages in this base repository or Drupal site.
 * 
 * If you are creating a base Drupal repository:
 * 
 *  * The packages are downloaded from Drupal through drush.
 * 
 *  * Packages are updated and recommited if they already exist in the 
 *    directory structure.
 * 
 * If you are creating a new Drupal site:
 *
 *  * The core platform and packages are pulled from their base repository path.
 *
 */
function drush_git_update() {
  git_include('gitcore');
  
  $path = drush_locate_root();  
  if (!GitCore::is_repository($path)) {
    return;
  }
  
  if (GitCore::get_config($path, 'drush.site') == 'false') {
    _git_update_base();
  }
  else {
    //_git_update_site();      
  }
}

//------------------------------------------------------------------------------

function _git_update_base() {  
	git_include('base');
  
  $drupal = new Base(NULL, FALSE);
  return $drupal->update();
}

//------------------------------------------------------------------------------

function _git_update_site() {
	
}

//------------------------------------------------------------------------------
// Internal utilities
//------------------------------------------------------------------------------

function git_include() {
	static $loaded = array();
	
	$args = func_get_args();
	$path = realpath(dirname(__FILE__) . '/includes');
	
	foreach ($args as $name) {
    $name = "git.$name";
  
    if (!$loaded[$name]) {    
      drush_include($path, $name, NULL, 'inc');
      $loaded[$name] = TRUE;
    }	
	}	
}

//------------------------------------------------------------------------------
