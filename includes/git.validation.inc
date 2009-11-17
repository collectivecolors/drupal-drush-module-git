<?php
//
// Git validation utilities.
//
//------------------------------------------------------------------------------

function git_validate_drupal_package($package) {
   
  if (!$package) {
    $package = 'drupal';
  }
  elseif (!preg_match('/^drupal(\-.+)?$/', $package)) {
    drush_set_error('DRUSH_GIT_PACKAGE_INVALID', dt('You have specified an invalid Drupal package name, @package', 
      array('@package' => $package))
    );
    return FALSE;  
  }
  return $package;    
}

//------------------------------------------------------------------------------

function git_validate_contrib_packages($packages) {
  
  if ($packages && !is_array($packages)) {
    $packages = array($packages);
  }
  
  if (!count($packages)) {
      drush_set_error('DRUSH_GIT_PACKAGE_INVALID', dt('You have not specified any package names'));
      return FALSE; 
  }
  
  foreach ($packages as $package) {
    // We must have specified a valid(ish) drupal package name. 
    if (!$package || preg_match('/^drupal(\-.+)?$/', $package)) {
      drush_set_error('DRUSH_GIT_PACKAGE_INVALID', dt('You have specified an invalid non core drupal project, @package', 
        array('@package' => $package))
      );
      return FALSE;  
    }
  }  
  return $packages; 
}

//------------------------------------------------------------------------------