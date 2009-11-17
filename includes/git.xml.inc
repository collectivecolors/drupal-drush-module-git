<?php
//
// Git general XML utilities.
//
//------------------------------------------------------------------------------

function git_xml_to_array($xml) {
  
  $xml = (array)$xml;
  foreach ($xml as $key => $data) {
    if (is_array($data) || $data instanceof SimpleXMLElement) {
      $xml[$key] = git_xml_to_array($data);
    }
  } 
  return $xml;
}

//------------------------------------------------------------------------------