<?php
//
// Git core command utilities. (mainly PHP wrappers)
//
//------------------------------------------------------------------------------

/**
 * Git branch listing modes.
 * 
 * We can either return the current branch, all branches ( as an array ), or
 * all branches and the current branch in a hash.
 */
define('GIT_BRANCH_ALL', 'all');
define('GIT_BRANCH_CURRENT', 'current');
define('GIT_BRANCH_COMBINED', 'combined');

//------------------------------------------------------------------------------

function git_is_repository($path) {
  // TODO: Make the Git check more thorough?
  return ($path && is_dir("$path/.git"));
}

//------------------------------------------------------------------------------

function git_branches($path, $mode = GIT_BRANCH_ALL, $clear = FALSE) {
  static $branches = array();
  
  if ($clear) {
    $branches = array();
  }
  
  if (!git_is_repository($path)) {
    return array();
  }
  
  if (!array_key_exists($path, $branches)) {
  	git_include('shell');
  	
    $list = git_shell_capture($path, 'git branch');
      
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
  
  if ($mode == GIT_BRANCH_CURRENT) {
    return $branches[$path]['current'];
  }
  elseif ($mode == GIT_BRANCH_COMBINED) { 
    return $branches[$path];
  }
  
  return $branches[$path]['branches'];
}

//------------------------------------------------------------------------------

function git_checkout_branch($path, $branch, $original = 'master') {
    
  if (!git_is_repository($path)) {
    return FALSE;
  }  
  git_include('shell');
  
  $branches = array_fill_keys(git_branches($path, GIT_BRANCH_ALL, TRUE), TRUE);
  
  if ($branches[$branch]) {
    $success = git_shell_exec($path, "git checkout '$branch'");
  }
  else {
    $success = git_shell_exec($path, "git checkout -b '$branch' '$original'");
  }
    
  return $success;
}

//------------------------------------------------------------------------------

function git_tags($path, $clear = FALSE) {
  static $tags = array();
  
  if ($clear) {
    $tags = array();
  }
    
  if (!git_is_repository($path)) {
    return array();
  }
  
  if (!array_key_exists($path, $tags)) {
    git_include('shell');    
    $tags[$path] = git_shell_capture($path, 'git tag');
  }
  
  return $tags[$path];
}

//------------------------------------------------------------------------------

function git_checkout_tag($path, $tag) {
    
  if (!git_is_repository($path)) {
    return FALSE;
  }  
  git_include('shell');
  
  $tags = array_fill_keys(git_tags($path, TRUE), TRUE);
  
  if ($tags[$tag]) {
    return git_shell_exec($path, "git checkout '$tag'");
  }
  return FALSE;
}

//------------------------------------------------------------------------------