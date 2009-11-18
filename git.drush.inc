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
 *   You can copy this file to any of the following
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

  $items['git init'] = array(
    'callback' => 'drush_git_init',
    'description' => "Initialize a Drupal base repository or a site.",
    'arguments' => array(
      'origin' => 'Origin url of the base repository for this site.  Not used on base installations.',
      'directory' => 'Directory to put this Drupal site into.  Defaults to "drupal-$VERSION".',
    ),
    'options' => array(
      '--tag' => 'Checkout the specific tag of the requested project package.  Note that this will create a detached head.',
      '--base' => 'Specify this flag if you are initializing a base Drupal repository.',
      '--package' => 'A specific package version of drupal to download. Only used when "--base" option is specified.',
      '--mode' => 'The download and commit mode for this project. Allowed values are; "all", "nodev", and "onlydev"',      
    ),
    'examples' => array(
      'git init git://example.com/drupal-6' => 'Initialize a new Drupal site into a "drupal-$VERSION" directory.',
      'git init git://example.com/drupal-6 mysite' => 'Initialize a new Drupal site into the "mysite" directory.',
      'git init --base drupal' => 'Initialize a base repository into the "drupal" directory.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  
  $items['git add'] = array(
    'callback' => 'drush_git_add',
    'description' => "Install or upgrade Drupal packages in a base repository or site.",
    'arguments' => array(
      'packages' => 'A space separated list of project names, with optional version.',
    ),
    'examples' => array(
      'git add diff-6.x-2.0' => 'Download a specfic version of diff module.',
      'git add cck zen' => 'Download latest versions of Zen for my version of Drupal.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT,
  );  
  
  $items['git update'] = array(
    'callback' => 'drush_git_update',
    'description' => "Update Drupal repository and all sub packages.",
    'examples' => array(
      'git update' => 'Update all projects in base Drupal repository or site', 
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_DRUSH,
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
    case 'drush:git init':
      return dt("Initialize a Drupal base repository or a site.");
      
    case 'drush:git add':
      return dt("Install or upgrade Drupal packages in a base repository or site.");
      
    case 'drush:git update':
      return dt("Update Drupal repository and all sub packages.");
  }
}

//------------------------------------------------------------------------------
// Callbacks

/**
 * git init
 * 
 * drush git init --base [ --package=$PACKAGE ]
 *                $BASE_URL
 *
 * Initialize a Base Drupal directory or a new Drupal site.
 * 
 * If you are creating a base Drupal repository:
 * 
 *  * The core platform and packages are downloaded from Drupal through drush.
 * 
 *  * Package directories contain totally separate ( no submodule ) repositories
 *    that are not cloned with the core platform.
 * 
 * If you are creating a new Drupal site:
 * 
 *  * The core platform is cloned from the base repo.
 *
 */
function drush_git_init() {
	  
  // Choose handler based on value of "--base".
  if (drush_get_option('base', FALSE)) {
  	_git_init_base();    
  }
  else {
  	$args = func_get_args();
  	_git_init_site($args[0]);
  }  
}

//------------------------------------------------------------------------------

/**
 * Initialize a base Drupal repository.
 */
function _git_init_base() {	
	git_include('validation', 'base.project');
	
	$package = git_validate_drupal_package(drush_get_option('package'));
	
	if ($package) {
    git_project_add($package,
      drush_get_option('mode', GIT_LIST_NO_DEV),
      !drush_get_option('tag', FALSE)
    );
	}
}

//------------------------------------------------------------------------------

/**
 * Initialize a Drupal site from a base Drupal repository.
 */
function _git_init_site($base_url, $directory = '') {
	
	if (!$directory) {
		$directory = '.';
	}
	
	// Default site information.
	$site_url = preg_replace('/\/?$/', '', $base_url) . '/sites/default';
	  
	// Initialize Drupal site.
	drush_shell_exec("git clone '$base_url' '$directory'");	
	  
  if (($directory == '.' || chdir($directory)) && is_dir('.git')) {
  	// Clone the default site directory.
  	drush_shell_exec("git clone '$site_url' 'sites/default'");
  	
  	// Mark this as a non base repository for add and update commands.
    drush_shell_exec("git config drush.site true");
  }
}

//------------------------------------------------------------------------------

/**
 * git add
 * 
 * drush git add [ --destination $DIRECTORY ] $PACKAGE ...
 *               [ --ssh-url $URL ] $PACKAGE ...
 *
 * Install or update packages in a base drupal directory or a new Drupal site.
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
 *  * All packages are added as submodules to the core repository from the 
 *    base repo.
 *
 */
function drush_git_add() {
	
  // Check for Git repository.
	if (!_git_cd_top_dir()) {
		return;
	}
	
  $args = func_get_args();
  
  if (shell_exec("git config drush.site") != 'true') {
  	_git_add_base($args);
  }
  else {
    _git_add_site($args);      
  }  
}

//------------------------------------------------------------------------------

function _git_add_base($packages) {
	
	if (!_git_validate_contrib_packages($packages)) {
      return;
  }
  
  foreach ($packages as $package) {
  	
  	$info = _git_project_info($package);
    
  	$project = $info['name'];
    $path    = $info['path'];
    
    if (is_null(($release = _git_project_release($package)))) {
    	return;
    }
    $version = $release['version'];  	
  	
    drush_print("Installing $project ( $version ) into $path");
    if (_git_project_download($info, $release)) {      
      
      if (!is_dir("$path/.git")) {
        drush_shell_exec("git init '$path'");
      }
        
      // Copy all hooks over from ~/hooks (if they exist)
      _git_copy_hooks($path);
        
      // Add all files and commit.
      drush_shell_exec(
          "git --git-dir='$path/.git' add .;"
        . " git --git-dir='$path/.git' commit -m 'Project $project - $version.';"
        . " git --git-dir='$path/.git' tag $version 2>/dev/null;"
      );
    }     
      
    // Record this project in git.  Do this at the very end!!!
    drush_shell_exec("git config drush.project.$project.path $path");
    drush_shell_exec("git config drush.project.$project.version $version"); // Current version checked out.  	  
  }
}

//------------------------------------------------------------------------------

function _git_add_site($packages) {
	
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

  // Check for Git repository.
  /*if (!_git_cd_top_dir()) {
    return;
  }*/  
  
  //if (shell_exec("git config drush.site") != 'true') {
    //_git_update_base();
    foreach (func_get_args() as $package) {
    	_git_project_download($package);
    }
  //}
  //else {
    //_git_update_site();
  //}
}

//------------------------------------------------------------------------------

function _git_update_base() {  
	$projects = _git_load_projects();
	_git_add_base($projects);	
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
