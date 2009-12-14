<?php
// $Id$

git_include('project', 'gitcore');

class Site extends Project
{
  //****************************************************************************
  // Constants
  //****************************************************************************

	
	
  //****************************************************************************
  // Properties
  //****************************************************************************
	
  protected $sub_projects         = array();
  protected $projects_initialized = FALSE;
  
  //****************************************************************************
  // Initialization
  //****************************************************************************
  
  function __construct($mode = NULL, $update_projects = TRUE) {
    parent::__construct('drupal', $mode);
    
    $this->init_projects(TRUE, $update_projects);
  }  
  
  //----------------------------------------------------------------------------
  
  public function init_projects($refresh = FALSE, $update_projects = TRUE) {
    
    if (!$refresh && $this->projects_initialized) {
      return TRUE;
    }   
    $path         = $this->get_path();
    $project_info = array();
    
    foreach (GitCore::capture($path, 'git submodule status', TRUE, TRUE) as $submodule) {
      $matches = array();
            
      if (preg_match('/^([^\s]+)\s+([^\s]+)/', $submodule, $matches)) {
        //$sha1_hash      = $matches[1];
        $submodule_path = $matches[2];
           
        $project_info[] = $submodule_path;          
      }
    }      
    foreach ($project_info as $submodule_path) {
      $instance = new Project($submodule_path, parent::ID_PATH);
           
      if ($update_projects && !$instance->update()) {
        return FALSE;
      }     
      $this->sub_projects[$instance->get_name()] = $instance;
    }
    
    $this->projects_initialized = TRUE;
    return TRUE;
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
      $instance = new Project($project, NULL, $mode);
      
      $instance->set_name($project);
      $instance->set_suppress_path_init(TRUE);
      
      if (!$instance->load_info()) {
      	return FALSE;
      }
      
      GitCore::exec($this->get_path(), array(
        'git submodule add '
      
      ), TRUE, TRUE);
      
      
      
      
      
      /*if (!$instance->update()) {
        return FALSE;
      }     
      $this->sub_projects[$project] = $instance;
        
      if (!GitCore::set_config($this->get_path(), 
        "drush.project.$project.path", 
        $instance->get_path())) {
        
        return FALSE;
      }
      return $instance;*/
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
    
    foreach (GitCore::capture($path, 'git submodule status', TRUE, TRUE) as $submodule) {
      $matches = array();
            
      if (preg_match('/^([^\s]+)\s+([^\s]+)/', $submodule, $matches)) {
        $sha1_hash = $matches[1];
        $path      = $matches[2];
           
        $project_info[$path] = $sha1_hash;          
      }
    }      
    foreach ($project_info as $path => $hash) {
      $instance = new Project($path, parent::ID_PATH);
           
      if ($update_projects && !$instance->update()) {
        return FALSE;
      }     
      $this->sub_projects[$instance->get_name()] = $instance;
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
  
}