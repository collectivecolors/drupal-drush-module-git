<?php
//
// General XML utilities.
//
//------------------------------------------------------------------------------
class XMLUtils
{
  //----------------------------------------------------------------------------
  
	static function to_array($xml) {
  
    $xml = (array)$xml;
    foreach ($xml as $key => $data) {
      if (is_array($data) || $data instanceof SimpleXMLElement) {
        $xml[$key] = XMLUtils::to_array($data);
      }
    }  
    return $xml;
  }

  //----------------------------------------------------------------------------
}