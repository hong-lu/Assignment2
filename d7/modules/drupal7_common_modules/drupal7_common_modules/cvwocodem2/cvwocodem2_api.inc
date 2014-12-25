<?php
/*
 * Copyright (c) 2008-2013
 * Computing for Volunteer Welfare Organizations (CVWO)
 * National University of Singapore
 *
 * Permission is hereby granted, free of charge, to any person obtainin
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
// $Id:
require_once drupal_get_path('module', 'cvwocodem2') . '/cvwocodem2_constants.php';

/**
 * Initializes questions and sets default options for given questions, generally should be
 * called in a module's .install file
 *
 * ... // In .install...
 * $temparray = array('opt1val'=>'opt1','opt2val'=>'opt2',...,'optnval'=>'optn');
 * cvwocodem2_registerquestion('this_module',1,'this_question',$temparray);
 * ...
 *
 * @param $module_name
 *   The name of the calling module
 * @param $qntext
 *   The question text (identifier), must be unique within a calling module
 * @param $optarry
 *   The array of options, should be an associative array of ansval=>ansname, defined in the desired order
 * @return
 *   1 if succeeds, 0 if fails
 */
function cvwocodem2_registerquestion($module_name, $qnval, $optarr, $qntext = null) {
  $query_args = array(
                'module_name' => $module_name,
                'qnval' => $qnval,
                'date_modified' => time());

  if(!is_null($qntext)){
    $query_args['qntext'] = $qntext;
  }else{
    $query_args['qntext'] = $qnval;
  }

  $query_opts = array('return' => Database::RETURN_INSERT_ID);

  // Insert question and return qid that was autoincremented
  $qid = db_insert(CVWOCODEM2_QUESTIONS_TABLE, $query_opts)
            ->fields($query_args)
            ->execute();

  $weight = 0;
  foreach ($optarr as $key => $val) {

    if(is_array($val)){
      $first_aid = cvwocodem2_addoption($qid, $key, $val['data'],$weight++, -1);

      //insert second level option
      foreach ($val['children'] as $key2 => $val2){
        if(is_array($val2)) {

          $second_aid = cvwocodem2_addoption($qid, $key2, $val2['data'],$weight++, $first_aid);

          if(is_array($val2['children'])) {
            foreach($val2['children'] as $key3 => $val3) {

              if(is_array($val3)) {
                drupal_set_message("Code Maintenance Module can only support three levels of options ");
                return 0;
              } else{
                //insert third level option
                cvwocodem2_addoption($qid, $key3, $val3,$weight++, $second_aid);
              }
            }
          }
        }else{
          cvwocodem2_addoption($qid, $key2, $val2,$weight++, $first_aid);
        }

      }

    }else{
      cvwocodem2_addoption($qid, $key, $val,$weight++, -1);
    }
  }

  return 1;
}


/**
 * Deletes the row from the cvwocodem2 table which matches answer id
 * exactly. Otherwise returns an error. Calls codem_reorder() at the end to guarantee good ordering of options.
 *
 * @param $aid
 * @return
 *   1 if succeeds, 0 if fails
 *
function cvwocodem2_deleteoption($aid) {
  //delete immediate childern, run cron job to delete deeper level children
  db_query("DELETE FROM {".CVWOCODEM2_MODULE_NAME."_options} WHERE parent_id='%d'", $aid);
  db_query("DELETE FROM {".CVWOCODEM2_MODULE_NAME."_options} WHERE aid='%d'", $aid);
  return 1;
}
*/

/**
 * Set one option as default option
 * @param $aid answer_id
 * @param $parentID parent option, or omit to use the top level.
 * @param $module_name
 * @param $qntext
 * @return unknown_type
 */
function cvwocodem2_defaultopt($aid, $qid, $parentID = -1) {
  // Clear all options' default setting
  $clear_query = db_update(CVWOCODEM2_OPTIONS_TABLE)
                ->fields(array(
                    'defaultopt' => 0
                  ))
                ->condition('parent_id', $parentID, '=')
                ->condition('qid', $qid, '=')
                ->execute();

  // Set Default options
  $default_query = db_update(CVWOCODEM2_OPTIONS_TABLE)
                    ->fields(array(
                        'defaultopt' => 1
                      ))
                    ->condition('aid', $aid, '=')
                    ->execute();

  if($clear_query && $default_query) drupal_set_message("Default option has been updated");
}


/**
 * Unset default option
 * @param $aid answer_id
 * @param $parentID parent option
 * @param $module_name
 * @param $qntext
 * @return unknown_type
 */
function cvwocodem2_undefaultopt($aid) {
  // Set default
  $update = db_update(CVWOCODEM2_OPTIONS_TABLE)
            ->fields(array(
                'defaultopt' => 0
                ))
            ->condition('aid', $aid, '=')
            ->execute();

  if($update) drupal_set_message("Default option has been updated");
}

/**
 * Gets the default option among this set of answer IDs. If none is specified, this
 * will return the first option listed in the database.
 * 
 * @param integer $qid The Question ID to query
 * @param integer $parent_id The suboption to select, if specified.
 */
function cvwocodem2_getdefaultopt($qid, $parent_id = -1) {
  $query = db_select(CVWOCODEM2_OPTIONS_TABLE)
                ->fields(CVWOCODEM2_OPTIONS_TABLE, array(
                    'aid'
                  ))
                ->condition('parent_id', $parent_id, '=')
                ->condition('qid', $qid, '=')
                ->condition('defaultopt', 0, '<>')
                ->execute();

  if (($row = $query->fetchAssoc())) {
    return $row['aid'];
  }
  
  return db_select(CVWOCODEM2_OPTIONS_TABLE)
                ->fields(CVWOCODEM2_OPTIONS_TABLE, array(
                    'aid'
                  ))
                ->condition('parent_id', $parent_id, '=')
                ->condition('qid', $qid, '=')
                ->execute()
                ->fetchField();
}

/**
 * Get option text
 * @param $module module name
 * @param $qnval  question value
 * @param $ansval answer value
 *
 * @deprecated since version 2.0
 */
function cvwocodem2_getoptiontext($module, $qnval, $ansval) {
  $qids = db_select(CVWOCODEM2_QUESTIONS_TABLE, 'q')
          ->condition('module_name', $module, '=')
          ->condition('qnval', $qnval, '=')
          ->fields('q', array('qid'))
          ->execute()
          ->fetchCol();

  $result = db_select(CVWOCODEM2_OPTIONS_TABLE, 'o')
            ->condition('ansval', $ansval, '=')
            ->condition('qid', $qids, 'IN')
            ->fields('o', array('anstext'))
            ->execute()
            ->fetchAssoc();

  return $result['anstext'];
}

/**
 * Gets the option text given the question value and answer ID.
 *
 * @param string $module module name
 * @param string $qnval  question value
 * @param integer $ansval answer value
 * @return string The option (display) text for the question/answer ID.
 * @deprecated since version 2 Use cvwocodem2_getanstext($aid)
 */
function cvwocodem2_getoptiontext2($module, $qnval, $aid) {
  $query = db_select(CVWOCODEM2_QUESTIONS_TABLE, 'q');
  $options_table = $query->addJoin('INNER', CVWOCODEM2_OPTIONS_TABLE, 'o', 'o.qid=q.qid');
  $query = $query->condition('q.module_name', $module)
            ->condition('q.qnval', $qnval)
            ->condition($options_table . '.aid', $aid)
            ->fields($options_table, array('anstext'))
            ->execute();

  $result = $query->fetchAssoc();
  return $result['anstext'];
}

/**
 * Gets the option value given the module name and answer ID.
 *
 * @param string $module module name
 * @param integer $aid answer value
 * @return string The option (display) text for the question/answer ID.
 */
function cvwocodem2_getoptionvalue($module, $aid) {
  $query = db_select(CVWOCODEM2_QUESTIONS_TABLE, 'q');
  $options_table = $query->addJoin('INNER', CVWOCODEM2_OPTIONS_TABLE, 'o', 'o.qid=q.qid');
  $query = $query->condition('q.module_name', $module)
            ->condition($options_table . '.aid', $aid)
            ->fields($options_table, array('ansval'))
            ->execute();

  $result = $query->fetchAssoc();
  return $result['ansval'];
}

/**
 * Modifies the row from the cvwocodem2 table which matches ($module_name, $qntext, $anstext)
 * exactly. Otherwise returns an error. Calls codem_reorder() at the end to guarantee good ordering of options.
 *
 * @param $module_name
 *   The name of the calling module
 * @param $qntext
 *   The question text (identifier), must be unique within a calling module
 * @param $anstext
 *   The answer/option text
  * @param $newtext
 *   The new answer/option text
  * @param $ansval
 *   The answer/option value
  * @param $newval
 *   The new answer/option value
 * @return
 *   1 if succeeds, 0 if fails
 */
function cvwocodem2_modifyoption($aid, $newtext, $weight, $parentID) {
  $update = cvwo_update(CVWOCODEM2_OPTIONS_TABLE)
            ->fields(array(
                'anstext' => $newtext,
                'weight' => $weight,
                'date_modified' => time()))
            ->condition('aid', $aid, '=')
            ->execute();

  if($update) drupal_set_message("Option has been updated.");
  return $update;
}


/**
 * update title for question
 * @param $qid question id
 * @param $newval new title text
 */
function cvwocodem2_updatetitle($qid, $newval) {
  $update = db_update(CVWOCODEM2_QUESTIONS_TABLE)
            ->fields(array(
                'qntext' => $newval))
            ->condition('qid', $qid, '=')
            ->execute();
  return $update;
}

/**
 * addoption for a question
 * @param $qid question id
 * @param $newval option value
 * @param $newtext  option text
 * @param $weight   weight
 * @param $parentID  parent_id option
 */
function cvwocodem2_addoption($qid, $newval, $newtext, $weight, $parentID) {
  // Check whether there is already another option with the new name
  $or = db_or()
        ->condition('ansval', $newval, '=')
        ->condition('anstext', $newtext, '=');

  $count = db_select(CVWOCODEM2_OPTIONS_TABLE)
            ->condition($or)
            ->condition('qid', $qid, '=')
            ->countQuery()
            ->execute()
            ->fetchField();

  if ($count>1) {
    drupal_set_message(sprintf('<div style="color:red">There is already an option with the same option text/value "%s".</div>', $newval));
    return 0;
  }

  $opts = array('return' => Database::RETURN_INSERT_ID);
  $insert_query = db_insert(CVWOCODEM2_OPTIONS_TABLE, $opts)
                  ->fields(array(
                      'qid' => $qid,
                      'ansval' => $newval,
                      'anstext' => $newtext,
                      'date_modified' => time(),
                      'weight' => $weight,
                      'parent_id' => $parentID))
                  ->execute();
  return $insert_query;
}




/**
 * Deletes questions and all options for given questions, generally should be
 * called in a module's .install file
 *
 * ... // In .install...
 * cvwocodem2_delquestions('this_module');
 * ...
 *
 * @param $module_name
 *   The name of the calling module
 * @return
 *   1 if succeeds, 0 if fails
 */
function cvwocodem2_delquestions($module_name) {
  $qids = db_select(CVWOCODEM2_QUESTIONS_TABLE, 'q')
          ->fields('q', array('qid'))
          ->condition('module_name', $module_name, '=')
          ->execute()
          ->fetchCol();

  // Delete questions
  $del_questions = db_delete(CVWOCODEM2_QUESTIONS_TABLE)
                   ->condition('module_name', $module_name, '=')
                   ->execute();

  // TODO PLEASE USE A TRANSACTION HERE
  // Delete options first
  if (!empty($qids)) {
    $del_options = db_delete(CVWOCODEM2_OPTIONS_TABLE)
                  ->condition('qid', $qids, 'IN')
                  ->execute();

    return $del_options && $del_questions;
  } else {
    return (boolean)$del_questions;
  }
}

/**
 * Toggle enabled setting of the particular option
 *
 * @return
 *    1 if success (enabled), 2 if success (disabled), 0 if failed
 * @param object $module_name
 *    Name of module
 * @param object $qntext
 *    question text
 * @param object $anstext
 *    answer text
 */
function cvwocodem2_toggleoption($aid) {
  // Select the desired option for updating
  $disabled = db_select(CVWOCODEM2_OPTIONS_TABLE, 'o')
              ->fields('o', array('disabled'))
              ->condition('aid', $aid, '=')
              ->execute()
              ->fetchField();

  $disabled = 1 - $disabled;

  // Update
  $update = db_update(CVWOCODEM2_OPTIONS_TABLE)
            ->fields(array('disabled' => $disabled))
            ->condition('aid', $aid, '=')
            ->execute();

  // Return success
  if($disabled == 0) {
    return 1;
  } else {
    return 2;
  }

}

/**
 * Gets whether the given answer ID is enabled.
 * 
 * @param integer $aid
 * @return boolean
 */
function cvwocodem2_is_option_enabled($aid) {
  // Select the desired option for updating
  return !intval(db_select(CVWOCODEM2_OPTIONS_TABLE, 'o')
              ->fields('o', array('disabled'))
              ->condition('aid', $aid)
              ->execute()
              ->fetchField());
}

/**
 * Resolves a Question ID given a module name and question value.
 *
 * @param string $module_name The name of the module being queried.
 * @param string $qnval The Question Value to look up.
 * @return integer
 */
function cvwocodem2_getqid($module_name, $qnval) {
  $qid = db_select(CVWOCODEM2_QUESTIONS_TABLE, 'q')
          ->fields('q', array('qid'))
          ->condition('module_name', $module_name, '=')
          ->condition('qnval', $qnval, '=')
          ->execute()
          ->fetchField();
  return $qid;
}

/**
 * Resolves an Answer Text given an Answer ID
 *
 * @param integer $aid The Answer ID to look up.
 * @return string
 */
function cvwocodem2_getanstext($aid) {
  $cache = &drupal_static(__FUNCTION__);
  if (!isset($cache)) {
    $cache = array();
  }
  if (empty($aid)) {
    return null;
  }
  if (array_key_exists($aid, $cache)) {
    return $cache[$aid];
  }

  $anstext = db_select(CVWOCODEM2_OPTIONS_TABLE, 'o')
          ->fields('o', array('anstext'))
          ->condition('aid', $aid)
          ->execute()
          ->fetchField();

  $cache[$aid] = $anstext;
  return $anstext;
}

/**
 * Resolves an Answer ID from a Question ID and Answer Text.
 *
 * @param integer $qid
 * @param string $anstext
 * @return integer
 */
function cvwocodem2_getaid($qid, $anstext) {
  $aid = db_select(CVWOCODEM2_OPTIONS_TABLE, 'o')
          ->fields('o', array('aid'))
          ->condition('qid', $qid)
          ->condition('anstext', $anstext)
          ->execute()
          ->fetchField();
  return $aid;
}

/**
 * Resolves an Answer ID from a Question ID and Answer Value.
 *
 * @param integer $qid
 * @param string $ansval
 * @return integer
 */
function cvwocodem2_getaidfromansval($qid, $ansval) {
  $aid = db_select(CVWOCODEM2_OPTIONS_TABLE, 'o')
          ->fields('o', array('aid'))
          ->condition('qid', $qid)
          ->condition('ansval', $ansval)
          ->execute()
          ->fetchField();
  return $aid;
}

/**
 * Returns an array of options corresponding to the module name and question, in ascending
 * order of aid. Usage by other modules will generally be:
 *
 * ... // In form input declaration...
 * '#options' => cvwocodem2_getoptions('this_module','this_question');
 * ...
 *
 * @param $module_name
 *   The name of the calling module
 * @param $qnval
 *   The question text (identifier), must be unique within a calling module
 * @param $enabled_only [optional]
 *   Returned options includes disabled options? Default is no
 * @param $default_null [optional]
 *   To set the default value to be null? Default is true
 * @param $default_text [optional]
 *   The text show for null value, default is "Please Select"
 * @return
 *   ARRAY if succeeds, 0 if fails
 *
 * @deprecated since version 2.0 Use the cvwocodem2_getoptions2 function instead,
 *                               where the keys returned are the answer IDs.
 */
function cvwocodem2_getoptions($module_name, $qnval, $enabled_only = 0, $default_null = true, $default_text = "Please Select") {

  global $cvwocodem_defaultopt;
  $is_enabled = '';
  if($enabled_only == 0) {
    $is_enabled = ' AND disabled=0';
  }

  // Retrieve all rows in cvwocodem table matching ($module_name, $qntext)
  // TODO Assuming that {module_name, qnval} is unique, it is according to
  // live data but we should add this constraint into the database
  $qid = db_select(CVWOCODEM2_QUESTIONS_TABLE, 'q')
          ->fields('q', array('qid'))
          ->condition('module_name', $module_name, '=')
          ->condition('qnval', $qnval, '=')
          ->execute()
          ->fetchField();

  //Load Option

  $ans = db_select(CVWOCODEM2_OPTIONS_TABLE, 'o1');
  $ans->leftJoin(CVWOCODEM2_OPTIONS_TABLE, 'o2', 'o1.aid = o2.parent_id');
  $ans->leftJoin(CVWOCODEM2_OPTIONS_TABLE, 'o3', 'o2.aid = o3.parent_id');
  $ans->condition('o1.parent_id', -1, '=')
      ->condition('o1.qid', $qid, '=')
      ->orderBy('o1.aid', 'ASC');
  $ans->addField('o1', 'aid', 'aid1');
  $ans->addField('o1', 'ansval', 'ansval1');
  $ans->addField('o1', 'anstext', 'anstext1');
  $ans->addField('o1', 'defaultopt', 'defaultopt');
  $ans->addField('o2', 'ansval', 'ansval2');
  $ans->addField('o2', 'anstext', 'anstext2');
  $ans->addField('o3', 'ansval', 'ansval3');
  $ans->addField('o3', 'anstext', 'anstext3');

  if ($enabled_only == 0) {
    $ans->condition(
			db_or()
				->condition('o1.disabled', 0, '=')
				->condition('o1.disabled')
		)
        ->condition(
			db_or()
				->condition('o2.disabled', 0, '=')
				->condition('o2.disabled')
		)
        ->condition(
			db_or()
				->condition('o3.disabled', 0, '=')
				->condition('o3.disabled')
		);
  }

  $result = $ans->execute();
  $return_val = array();
  $textlen = 0; // To centralize the default text

  // If there is only one row, return that row
  if ($result->rowCount() == 1) {
    $result = $result->fetchObject();
    $side_length = ($textlen-strlen($default_text))/2;

    if($side_length>0) {
        for($i=0; $i<$side_length-2; $i++) {
            $text .='-';
        }
        $default_text = $text.' '.$default_text.' '.$text;
    }

    $return_val[''] = $default_text;
    $return_val[$result->ansval1] = $result->anstext1;
    return $return_val;
  }

  // If a node has children, it will be an array with this structure a
  // $parent[anstext] = array('data' => ansval, 'children')
  // else if it's a leaf, it will have $parent[anstext] = ansval
  while ($data = $result->fetchAssoc()) {
    // Add to defaultopt if is default
    if ($data['defaultopt'] == 1) {
      $cvwocode_defaultopt = array($data['ansval1'] => $data['anstext1']);
    }

    // Determine how deep this entry is
    $depth = is_null($data['ansval3']) ? (is_null($data['ansval2']) ? 1 : 2) : 3;

    // Check that the required array are already initialised, else initialise them
    switch ($depth) {
      case 2:
        if (!isset($return_val[$data['ansval1']])) {
          $return_val[$data['ansval1']] = array('data' => $data['anstext1'], 'children' => array());
          if ($textlen < strlen($data['anstext1'])) $textlen = strlen($data['anstext1']);
        }
        break;
      case 3:
        if (!isset($return_val[$data['ansval1']])) {
          $return_val[$data['ansval1']] = array(
            $data['ansval2'] => array('data' => $data['anstext1'], 'children' => array('data' => $data['anstext2'], 'children' => array()))
          );
          if ($textlen < strlen($data['anstext1'])) $textlen = strlen($data['anstext1']);
          if ($textlen < strlen($data['anstext2'])) $textlen = strlen($data['anstext2']);
        } else {
          if (!isset($return_val[$data['ansval1']]['children'][$data['ansval2']])) {
            $return_val[$data['ansval1']]['children'][$data['ansval2']] = array(
              'data' => $data['anstext2'], 'children' => array()
            );
          }
          if ($textlen < strlen($data['anstext1'])) $textlen = strlen($data['anstext1']);
          if ($textlen < strlen($data['anstext2'])) $textlen = strlen($data['anstext2']);
        }
    }

    // Insert the values
    switch ($depth) {
      case 1:
        $return_val[$data['ansval1']] = $data['anstext1'];
        if ($textlen < strlen($data['anstext1'])) $textlen = strlen($data['anstext1']);
        break;
      case 2:
        $return_val[$data['ansval1']]['children'][$data['ansval2']] = $data['anstext2'];
        if ($textlen < strlen($data['anstext2'])) $textlen = strlen($data['anstext2']);
        break;
      case 3:
        $return_val[$data['ansval1']]['children'][$data['ansval2']]['children'][$data['ansval3']] = $data['anstext3'];
        if ($textlen < strlen($data['anstext3'])) $textlen = strlen($data['anstext3']);
        break;
    }
  }

  // Append the default text in the front
  $text = '';
  if($default_null) {
      $side_length = ($textlen-strlen($default_text))/2;

      if($side_length>0) {
          for($i=0; $i<$side_length-2; $i++) {
              $text .='-';
          }
      }
      $default_text = $text.' '.$default_text.' '.$text;
      $return_val = array('' => $default_text) + $return_val;
  }

  return $return_val;
}

/**
 * Returns an array of options corresponding to the module name and question, in
 * ascending order of aid. Usage by other modules will generally be:
 *
 * ... // In form input declaration...
 * '#options' => cvwocodem2_getoptions2('this_module','this_question');
 * ...
 *
 * @param $module_name
 *   The name of the calling module
 * @param $qnval
 *   The question text (identifier), must be unique within a calling module
 * @param $include_disabled [optional]
 *   Returned options includes disabled options? Default is no
 * @param $default_null [optional]
 *   To set the default value to be null? Default is true
 * @param $default_text [optional]
 *   The text show for null value, default is "Please Select"
 * @return
 *   ARRAY if succeeds, 0 if fails
 * 
 * @todo This is a copy/paste of the cvwocodem2_getoptions call above, because
 *       existing code relies on the behaviour of the function. We should refactor
 *       given time.
 */
function cvwocodem2_getoptions2($module_name, $qnval, $include_disabled = 0, $default_null = true, $default_text = 'Please Select') {
  // Retrieve all rows in cvwocodem table matching ($module_name, $qntext)
  // TODO Assuming that {module_name, qnval} is unique, it is according to
  // live data but we should add this constraint into the database
  $qid = db_select(CVWOCODEM2_QUESTIONS_TABLE, 'q')
          ->fields('q', array('qid'))
          ->condition('module_name', $module_name, '=')
          ->condition('qnval', $qnval, '=')
          ->execute()
          ->fetchField();

  $options = cvwocodem2_getoptions_for_qid($qid, $include_disabled, $default_null, $default_text);
  
  // if ($default_null) {
  //     $options = array(null => $default_text) + $options;
  // }

  return $options;
}

/**
 * Helper function to center text using character provided
 *
 * @param $default_text
 *   Text to be padded
 * @param $side_length
 *   Width of the padding for a single side
 * @param $padding_char
 *   The character to pad with, defaults to hyphen
 * @return
 *   Padded string
 */
function centralise_text($text, $side_length, $padding_char='-') {
	$padding_text = '';
	for ($i=0; $i < $side_length - 2; $i++) {
		$padding_text .= $padding_char;
	}
	return $padding_text.' '.$text.' '.$padding_text;
}

/**
 * Returns an array of options corresponding to the module name and question, in
 * ascending order of aid. Usage by other modules will generally be:
 *
 * ... // In form input declaration...
 * '#options' => cvwocodem2_getoptions2('this_module','this_question');
 * ...
 *
 * @param $qid
 *   The Question ID to look up the options for
 * @param $include_disabled [optional]
 *   Returned options includes disabled options? Default is no
 * @param $default_null [optional]
 *   To set the default value to be null? Default is true
 * @param $default_text [optional]
 *   The text show for null value, default is "Please Select"
 * @return
 *   ARRAY if succeeds, 0 if fails
 *
 * @todo This is a copy/paste of the cvwocodem2_getoptions call above, because
 *       existing code relies on the behaviour of the function. We should refactor
 *       given time.
 */
function cvwocodem2_getoptions_for_qid($qid, $include_disabled = 0, $default_null = true, $default_text = "Please Select") {

  global $cvwocodem_defaultopt;
  $is_enabled = '';
  if (!$include_disabled) {
    $is_enabled = ' AND disabled=0';
  }

  //Load Option
  $ans = db_select(CVWOCODEM2_OPTIONS_TABLE, 'o1');
  $ans->leftJoin(CVWOCODEM2_OPTIONS_TABLE, 'o2', 'o1.aid = o2.parent_id');
  $ans->leftJoin(CVWOCODEM2_OPTIONS_TABLE, 'o3', 'o2.aid = o3.parent_id');
  $ans->condition('o1.parent_id', -1, '=')
      ->condition('o1.qid', $qid, '=')
      ->orderBy('o1.aid', 'ASC');
  $ans->addField('o1', 'aid', 'aid1');
  $ans->addField('o1', 'ansval', 'ansval1');
  $ans->addField('o1', 'anstext', 'anstext1');
  $ans->addField('o1', 'defaultopt', 'defaultopt');
  $ans->addField('o2', 'aid', 'aid2');
  $ans->addField('o2', 'ansval', 'ansval2');
  $ans->addField('o2', 'anstext', 'anstext2');
  $ans->addField('o3', 'aid', 'aid3');
  $ans->addField('o3', 'ansval', 'ansval3');
  $ans->addField('o3', 'anstext', 'anstext3');

  if ($include_disabled == 0) {
    $ans->condition(
			db_or()
				->condition('o1.disabled', 0, '=')
				->condition('o1.disabled')
		)
        ->condition(
			db_or()
				->condition('o2.disabled', 0, '=')
				->condition('o2.disabled')
		)
        ->condition(
			db_or()
				->condition('o3.disabled', 0, '=')
				->condition('o3.disabled')
		);
  }

  $result = $ans->execute();

  $option_text_len = 0;

  // If a node has children, it will be an array with this structure a
  // $parent[anstext] = array('data' => ansval, 'children')
  // else if it's a leaf, it will have $parent[anstext] = ansval
  $return_val = array();
  while ($data = $result->fetchAssoc()) {
    // Add to defaultopt if is default
    if ($data['defaultopt'] == 1) {
      $cvwocode_defaultopt = array($data['aid1'] => $data['anstext1']);
    }

    // Determine how deep this entry is
    $depth = is_null($data['ansval3']) ? (is_null($data['aid2']) ? 1 : 2) : 3;

    // Check that the required array are already initialised, else initialise them
    switch ($depth) {
      case 2:
        if (!isset($return_val[$data['anstext1']])) {
          $return_val[$data['anstext1']] = array('data' => $data['aid1'], 'children' => array());
          if ($option_text_len < strlen($data['anstext1']))
			$option_text_len = strlen($data['anstext1']);
        }
        break;
      case 3:
        if (!isset($return_val[$data['anstext1']])) {
          $return_val[$data['anstext1']] = array(
            $data['anstext2'] => array('data' => $data['aid1'], 'children' => array('data' => $data['ansval2'], 'children' => array()))
          );
        } else {
          if (!isset($return_val[$data['anstext1']]['children'][$data['anstext2']])) {
            $return_val[$data['anstext1']]['children'][$data['anstext2']] = array(
              'data' => $data['aid2'], 'children' => array()
            );
          }
        }
        if ($option_text_len < strlen($data['anstext1'])) $option_text_len = strlen($data['anstext1']);
		if ($option_text_len < strlen($data['anstext2'])) $option_text_len = strlen($data['anstext2']);
    }

    // Insert the values
    switch ($depth) {
      case 1:
        $return_val[$data['aid1']] = $data['anstext1'];
        if ($option_text_len < strlen($data['anstext1']))
			$option_text_len = strlen($data['anstext1']);
        break;
      case 2:
        $return_val[$data['aid1']]['children'][$data['anstext2']] = $data['ansval2'];
        if ($option_text_len < strlen($data['ansval2']))
			$option_text_len = strlen($data['ansval2']);
        break;
      case 3:
        $return_val[$data['aid1']]['children'][$data['anstext2']]['children'][$data['anstext3']] = $data['ansval3'];
        if ($option_text_len < strlen($data['ansval3']))
			$option_text_len = strlen($data['ansval3']);
        break;
    }
  }

  $default_text_len = strlen($default_text);

  // Append the default text in the front
  $text = '';
  if($default_null) {
	  $display_text = centralise_text($default_text, ($option_text_len - $default_text_len)/2);
	  $return_val = array('' => $display_text) + $return_val;
  }

  // TODO Clarify the structure of the codem
  return $return_val;
}

/**
 * Get options for multi-level options
 * @param $module_name
 * @param $qnval
 * @param $enabled_only
 * @param $default_null
 * @param $default_text
 */
function cvwocodem2_getmultileveloptions($module_name, $qnval, $enabled_only = 0, $default_null = true, $default_text = "Please Select") {

    $data = cvwocodem2_getoptions($module_name, $qnval, $enabled_only, $default_null);

    $result = array();
    foreach($data as $level1key => $level1) {
      if(isset($level1['children']) && is_array($level1['children'])){
        $level2 = array();
        foreach($level1['children'] as $level2key=> $level2val){
          if(isset($level2val['children']) && is_array($level2val['children']))
            $level2[$level2key] = $level2val['children'];
          else
            $level2[$level2key] = $level2val;
        }
        $result[$level1key] = $level2;
      }else{
        $result[$level1key] = $level1;
      }
    }
    return $result;
}


/**
 * Get default option of the last getOptions function call
 * @return array of key->val
 */
function cvwocodem2_get_lastdefaultoption()
{
  return $cvwocodem2_defaultopt;
}


function cvwocodem2_getsuboptions($module_name, $qntext, $parentval, $disabled = 0, $default_null = false, $default_text = "Please Select") {

  global $cvwocodem2_defaultopt;

  $result = db_select(CVWOCODEM2_OPTIONS_TABLE,'o1');
  $result->join(CVWOCODEM2_OPTIONS_TABLE, 'o2', 'o1.aid = o2.parent_id');
  $result->condition('o1.ansval', $parentval, '=')
         ->condition('o1.module_name', $module_name, '=')
         ->condition('o1.qntext', $qntext, '=')
         ->fields('o1',array())
         ->fields('o2')
         ->orderBy('o2.weight', 'ASC');

  if ($disabled == 0) {
    $result->condition('o1.disabled', 0, '=');
    $result->condition('o2.disabled', 0, '=');
  }

  $result->execute();

  if ($result->rowCount() == 0) {
    drupal_set_message('<div style="color:red">Parent option value doesn\'t exists.</div>');
    return 0;
  }

  $nodecount=0;
  $maxlength = 0;
  $res = array();
  foreach ($result as $node) {
    $res[$node->ansval] = $node->anstext;
    if(strlen($node->anstext)>$maxlength) {
      $maxlength = strlen($node->anstext);
    }
    if($node->defaultopt == 1) {
      $cvwocodem2_defaultopt = array($node->ansval => $node->anstext);
    }
    $nodecount++;
  }

  // If no such rows, return an error
  if ($nodecount==0) {return 0;}

  //format the text displayed for null value
  if($default_null) {
    $side_length = ($maxlength-strlen($default_text))/2;
    if($side_length>0) {
      for($i=0; $i<$side_length-2; $i++)
             $text .='-';
       $default_text = $text.' '.$default_text.' '.$text;
    }
    $res = array(0 => $default_text) + $res;
  }

  return $res;
}