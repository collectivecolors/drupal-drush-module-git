<?php
// $Id$

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
      '--base' => 'Specify this flag if you are initializing a base Drupal repository.',
      '--package' => 'A specific package version of drupal to download. Only used when "--base" option is specified.',
    ),
    'examples' => array(
      'git init git://example.com/drupal-6' => 'Initialize a new Drupal site into a "drupal-$VERSION" directory.',
      'git init git://example.com/drupal-6 mysite' => 'Initialize a new Drupal site into the "mysite" directory.',
      'git init --base drupal' => 'Initialize a base repository into the "drupal" directory.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
  
  $items['git dl'] = array(
    'callback' => 'drush_git_dl',
    'description' => "Install or upgrade Drupal packages in a base repository or site.",
    'arguments' => array(
      'projects' => 'A space separated list of project names, with optional version.',
    ),
    'options' => array(
      '--base' => 'Specify this flag if you are installing to a base Drupal repository.',
      '--destination' => 'Path to which the project will be copied.  Only used when "--base" option is specified.',
      '--ssh-url' => 'Url of the base repository to syncronize to.  Not used when "--base" option is specified.',
    ),
    'examples' => array(
      'git dl' => 'Download latest version of Drupal packages.', 
      'git dl diff-6.x-2.0' => 'Download a specfic version of diff module.',
      'git dl cck zen' => 'Download latest versions of Zen for my version of Drupal.',
      'git dl --ssh-url you@example.com:/repo/drupal devel' => 'Download latest version of devel and update base repository first if package does not exist.',
      'git dl --base admin_menu devel cck views' => 'Download latest versions of these modules into the current base repository.'
    ),
  );  
  
  $items['git update'] = array(
    'callback' => 'drush_git_update',
    'description' => "Update Drupal repository and all sub packages.",
    'arguments' => array(
      'modules' => 'Optional. A space delimited list of installed projects to update (currently only modules).',
    ),
    'options' => array(
      '--base' => 'Specify this flag if you are installing to a base Drupal repository.',
      '--backup-dir' => 'Specify a directory to backup packages into, defaults to a backup directory within your Drupal root.',
    ),
    'examples' => array(
      'git update' => 'Update all projects in Drupal site', 
      'git update --base' => 'Update all Drupal packages in the base Drupal repository.',
    ),
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
      
    case 'drush:git dl':
      return dt("Install or upgrade Drupal packages in a base repository or site.");
      
    case 'drush:git update':
      return dt("Update Drupal repository and all sub packages.");
  }
}

//------------------------------------------------------------------------------
// Callbacks

/**
 * Git init
 * 
 * drush git init --base [ --version $PACKAGE ] [ $DIRECTORY ]
 *                $BASE_URL [ $DIRECTORY ]
 *
 * Initialize a Base drupal directory or a new Drupal site.
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
	 // Bootstrap to the highest level possible.
  drush_bootstrap_max();

  drush_include_engine('package_handler', drush_get_option('package-handler', 'wget'));
	
  // Initialize arguments.
  $is_base = drush_get_option('base');
  $args    = func_get_args();
  
  // Choose handler based on value of "--base".
  if (!empty($is_base)) {
  	_git_init_base(drush_get_option('package'), $args[0]);    
  }
  else {
  	_git_init_site($args[0], $args[1]);
  }  
}

//------------------------------------------------------------------------------

/**
 * Initialize a base Drupal repository.
 */
function _git_init_base($package, $directory) {
	
	// We must have specified a valid(ish) drupal package name. 
	if (!$package) {
		$package = 'drupal';
	}
	elseif (!preg_match('/^drupal\-/', $package)) {
    drush_set_error('DRUSH_GIT_PACKAGE_INVALID', dt('You have specified an invalid package name, @package', 
      array('@package' => $package))
    );	
	}
	
	// Download drupal.
	list($project, $release, $destination) = _git_package_download($package);
		
	if (!is_null($project)) {
    $version   = $release['version'];
    
    $orig_dest = $destination . "$project-$version";
    $new_dest  = $orig_dest;
    
    print "$project\n";
    print "$version\n";
    print "$orig_dest\n";
    
    if ($directory) {
    	$new_dest = $directory;
    }
    
    //mkdir($new_dest, 0755, TRUE);
    if ($orig_dest != $new_dest) {
      drush_shell_exec("mv $orig_dest $new_dest");
    }
    
    if (chdir($new_dest)) {
    	// We are at the new site.
    	print getcwd() . "\n";
    	
    	// Intialize sites/all/modules directory.
    	mkdir('sites/all/modules', 0755, TRUE);
    	
    	$fh = fopen("sites/all/modules/README.txt", 'w');
    	fwrite($fh, "Put all shared modules for sites on this platform here.\n");
    	fclose($fh);
    	
    	// Initialize sites/all/themes directory.
    	mkdir('sites/all/themes', 0755, TRUE);
    	
      $fh = fopen("sites/all/themes/README.txt", 'w');
      fwrite($fh, "Put all shared themes for sites on this platform here.\n");
      fclose($fh);
      
      // Initialize sites/all/libraries directory.
    	mkdir('sites/all/libraries', 0755, TRUE);
    	
      $fh = fopen("sites/all/libraries/README.txt", 'w');
      fwrite($fh, "Put all shared libraries for sites on this platform here.\n");
      fclose($fh);
      
      // Initialize Git repository.
      drush_shell_exec("git init");
      
      // Set up public ignores.
      $fh = fopen(".gitignore", 'w');
      fwrite($fh, 
          "# Ignore data directory.\n"
        . "sites/default\n"
      );
      fclose($fh);
      
      // Set up private ignores.
      $fh = fopen(".git/info/exclude", 'a');
      fwrite($fh, 
          "\n# Ignore everything in modules, themes, and library directories except README.txt\n"
        . "sites/all/modules/*\n"
        . "!sites/all/modules/README.txt\n\n"
        . "sites/all/themes/*\n"
        . "!sites/all/themes/README.txt\n\n"
        . "sites/all/libraries/*\n"
        . "!sites/all/libraries/README.txt\n\n"
      );
      fclose($fh);
      
      // Copy all hooks over from ~/hooks (if they exist)
      _git_copy_hooks();
      
      # Add all files and commit.
      drush_shell_exec(
          "git add .;"
        . " git commit -m 'Drupal $version';"
        . " git tag $version;"
      );
      
      # Initialize default site data directory.
      if (chdir('sites/default')) {
        // We are in the data directory.
        print getcwd() . "\n";
        
        // Initialize site data file directory.
        mkdir('files', 0777, TRUE);
        
        // Set up public ignores.
        $fh = fopen(".gitignore", 'w');
        fwrite($fh, 
          "All shared platform files go here.\n\n"
        . "Sync the backup of this directory with the database backup schedule.\n"
        );
        fclose($fh);
        
        // Allow world write persmissions on default settings file.
        // This is changed back later ( after initialization ).
        chmod("default.settings.php", 0755);
        
        // Initialize git repository for the data directory.
        //
        // We keep this directory separate from the site so we can have different sites
        // powered by the same platform.  We can always clone this directory to create
        // new sites though.
        drush_shell_exec("git init");
        
        // Copy all hooks over from ~/hooks (if they exist)
        _git_copy_hooks();
        
        // Add all files and commit.
        drush_shell_exec(
            "git add .;"
          . " git commit -m 'Default site data directory.';"
        );      
      }      
    }
    else {
    	drush_set_error('DRUSH_GIT_CHDIR_FAILED', dt('Unable to change directory to, @dest', 
        array('@dest' => $new_dest))
      );
    }
	}	
}

//------------------------------------------------------------------------------

/**
 * Initialize a Drupal site from a base Drupal repository.
 *
 * @param unknown_type $base_url
 * @param unknown_type $directory
 */
function _git_init_site($base_url, $directory) {
	
}

//------------------------------------------------------------------------------

/**
 * Git dl
 * 
 * drush git dl [ -b ] [ -s SSH_URL ] PACKAGE ...
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
function drush_git_dl() {
  $commands = func_get_args();
  
  
}

//------------------------------------------------------------------------------

/**
 * Git update
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
  $commands = func_get_args();
  
  
}

//------------------------------------------------------------------------------
// Internal utilities
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

function _git_package_download($package_name) {
  $requestdata = pm_parse_project_version(array($package_name));
  
  $project_types = pm_project_types();
  $project_types_xpath = '(value="' . implode('" or value="', $project_types) . '")';
  
  // Return variables;
  $project     = NULL;
  $release     = '';
  $destination = '';
  
  foreach ($requestdata as $package) {
    $project = $package['name'];
    // Don't rely on UPDATE_DEFAULT_URL since we are not fully bootstrapped.
    $url = 'http://updates.drupal.org/release-history' . "/$project/". $package['drupal_version'];
    if ($xml = @simplexml_load_file($url)) {
      if ($error = $xml->xpath('/error')) {
        drush_set_error('DRUSH_PM_COULD_NOT_LOAD_UPDATE_FILE', $error[0]);
      }
      else {
        // Try to get the specified release.
        if ($package['version']) {
          if ($releases = $xml->xpath("/project/releases/release[status='published'][version='" . $package['version'] . "']")) {
            $release = (array)$releases[0];
          }
          if (empty($release)) {
            drush_log(dt("Could not locate specified project version, downloading latest stable version"), 'notice');
          }
        }
        // If that did not work, get the first published release for the recommended major version.
        if (empty($release)) {
          $recommended_major = $xml->xpath("/project/recommended_major");
          $xpath_releases = "/project/releases/release[status='published'][version_major=" . (string)$recommended_major[0] . "]";
          $releases = $xml->xpath($xpath_releases);
          $release = (array)$releases[0];
        }
        // Determine what type of project we have, so we know where to put it.
        $release['type'] = 'module';

        if ($types = $xml->xpath('/project/terms/term[name="Projects" and ' . $project_types_xpath . ']')) {
          $release['type'] = array_search($types[0]->value, $project_types);
        }

        if ($destination = pm_dl_destination($release['type'])) {
          if (package_handler_install_project($project, $release, $destination)) {
            drush_log(dt("Project !project (!version) downloaded to !dest.",
                         array('!project' => $project, '!version' => $release['version'], '!dest' => $destination)), 'success');
            drush_command_invoke_all('drush_pm_post_install', $project, $release, $destination);
          }
        }
      }
    }
    else {
      // We are not getting here since drupal.org always serves an XML response.
      drush_set_error('DRUSH_PM_DOWNLOAD_FAILED', dt('Could not download project status information from !url', array('!url' => $url)));
    }
    
    break; # This should not be needed.    
  }
  return array($project, $release, $destination);  
}

//------------------------------------------------------------------------------

/**
 * Return all hooks in a specified directory. 
 * 
 * Defaults to hooks directory under current user home directory.
 */
function _git_get_hooks($directory) {
  $hooks = array();
  
  $dh = opendir($directory);
  while ($file = readdir($dh)) {

    // if $file isn't this directory or its parent, 
    // add it to the hooks array
    if ($file != '.' && $file != '..') {
      $hooks[] = $file;
    }
  }

  closedir($dh);
  
  return $hooks;
}

//------------------------------------------------------------------------------

/**
 * Copy all hooks defined in the users home hooks directory to current repository.
 */
function _git_copy_hooks() {
  $hook_dir = $_SERVER['HOME'] . '/hooks';
  $hooks    = _git_get_hooks($hook_dir);
      
  foreach ($hooks as $file) {
    copy("$hook_dir/$file", ".git/hooks/$file");
    chmod(".git/hooks/$file", 0755);
  }
}

