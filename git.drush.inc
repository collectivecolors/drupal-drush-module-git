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
    ),
    'examples' => array(
      'git init git://example.com/drupal-6' => 'Initialize a new Drupal site into a "drupal-$VERSION" directory.',
      'git init git://example.com/drupal-6 mysite' => 'Initialize a new Drupal site into the "mysite" directory.',
      'git init --base drupal' => 'Initialize a base repository into the "drupal" directory.',
    ),
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
 * drush git init ( [ -b ] | [ -r URL ] ) DIRECTORY
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
  $commands = func_get_args();
  
  
}

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
