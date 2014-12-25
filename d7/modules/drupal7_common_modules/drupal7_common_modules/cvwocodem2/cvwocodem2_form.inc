<?php
/**
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
// $Id: cvwo_notification_api.inc 3917 2010-02-21 09:49:23Z wangsha $
require_once drupal_get_path('module', 'cvwocodem2') . '/cvwocodem2_constants.php';
require_once drupal_get_path('module', 'cvwocodem2') . '/cvwocodem2_api.inc';

/**
 * Converts a module name (identifier) to the title defined in the .info file.
 *
 * @param string $module The name of the module
 * @return string The title of the module
 */
function cvwocodem2_module_name_to_title($module) {
  if (empty($module)) {
    return '';
  }
  $path = drupal_get_path('module', $module) . '/' . $module . '.info';
  $info = drupal_parse_info_file($path);
  return isset($info['name']) ? $info['name'] : $module;
}

function cvwocodem2_form($form, &$form_state = null) {
  $form = array();
  // Load modules
  $modules = db_select(CVWOCODEM2_QUESTIONS_TABLE, 'q')
              ->fields('q', array('module_name'))
              ->distinct()
              ->execute()
              ->fetchCol();

  if (!empty($form_state['post'])) {
    $values = $form_state['post'];
  } else if(!empty($form_state['values'])) {
    $values = $form_state['values'];
  } else {
    $values = null;
  }

  $hiddensubmit = array(
    '#type' => 'submit',
    '#name' => 'hidden',
    '#value' => 'hiddensubmit',
    '#prefix' => '<span style="display:none">',
    '#suffix' => '</span>'
  );

  $form['module'] = array(
    '#type' => 'select',
    '#title' => t('Module'),
    '#options' => drupal_map_assoc(array_merge(array(''), $modules),
    'cvwocodem2_module_name_to_title'),
    '#default_value' => $values['module'],
    '#description' => t('Select module to modify'),
    '#name' => 'module',
    '#prefix' => '<table><tr><td>',
    '#suffix' => '</td>',
    '#attributes'=>array('onchange'=>'form.submit("module")'),
    '#width' => 30,
  );

  if(empty($values['module'])) {
	$hiddensubmit['#prefix'] = '<td style="display: none">' . $hiddensubmit['#prefix'];
    $hiddensubmit['#suffix'] .= '</td></tr></table>';
    $form['hiddensubmit'] = $hiddensubmit;
    return $form;
  }

  //Load Questions
  $questions = db_select(CVWOCODEM2_QUESTIONS_TABLE, 'q')
                ->fields('q', array('qid', 'qntext'))
                ->condition('q.module_name', $values['module'])
                ->execute();

  $questionarray = array('' => '');
  foreach($questions as $data) {
    $questionarray[$data->qid] = $data->qntext;
  }

  $form['question'] = array(
    '#type' => 'select',
    '#title' => t('Questions'),
    '#options' => $questionarray,
    '#name' => 'question',
    '#default_value' => (empty($values['question']) ? '' : $values['question']),
    '#description' => t('Select Question to modify'),
    '#prefix' => '<td>',
    '#suffix' => '</td></tr></table>',
    '#attributes'=>array('onchange'=>'form.submit("question")'),
    '#states' => array('invisible' => array(':input[name=module]' => array('value' => '')))
  );

  $form['hiddensubmit'] = $hiddensubmit;

  // hide elements when there is no questions
  $invisible_state = array(':input[name=question]' => array('value' => ''));
  if (isset($values['question']) && isset($questionarray[$values['question']])) {
    $default_title = $questionarray[$values['question']];
  } else {
    $default_title = '';
    $values['question'] = '';
  }

  $form['qtitle'] = array(
    '#type' => 'textfield',
    '#title' => t('Change Question Text'),
    '#default_value' => $default_title,
    '#value' => $default_title,
    '#description'=>t("Please enter new question title"),
    '#size'=> 20,
    '#prefix' => '<td>',
    '#suffix' => '</td>',
    '#states' => array('invisible' => $invisible_state)
  );

  $form['qtitle_submit'] = array(
    '#type' => 'submit',
    '#name' => 'qtitle_'.$values['question'],
    '#value' => t('Update'),
    '#prefix' => '<td>',
    '#suffix' => '</td></tr></table>',
    '#states' => array('invisible' => $invisible_state)
  );

  $options = cvwocodem2_getoptions($values['module'], $questionarray[$values['question']], 0, false);

  //Load Option

  /*
   * retrieve root options
   */
  $pids = db_select(CVWOCODEM2_OPTIONS_TABLE, 'o')
          ->condition('o.qid', $values['question'], '=')
          ->condition('o.parent_id', -1, '=')
          ->fields('o')
          ->execute();

  /*
   * hidden values
   */
  $form['qid'] = array(
    '#type'=>'hidden',
    '#value'=>$values['question'],
  );

  $firstlevel = array();
  $root = array();
  while ($node = $pids->fetchObject()) {
    $firstlevel[$node->aid]=$node;
    $root[$node->aid]=array();
    $root[$node->aid]['data']=$node;
  }

  /*
   * retrieve sencond level options
   */
  $secondlevel = array();
  $secondtofirst = array();
  foreach($firstlevel as $pid) {
    $second = db_select(CVWOCODEM2_OPTIONS_TABLE, 'o')
              ->condition('o.parent_id', $pid->aid, '=')
              ->fields('o')
              ->orderBy('o.weight', 'ASC')
              ->execute();

    while ($node = $second->fetchObject()) {
      $secondlevel[$node->aid]=$node;
      $secondtofirst[$node->aid] = $node->parent_id;
      $root[$node->parent_id]['children'][$node->aid] = array('data'=>$node);
    }
  }

  /*
   * retrieve third level options
   */
  $thirdlevel = array();
  foreach($secondlevel as $pid) {
    $third = cvwo_select(CVWOCODEM2_OPTIONS_TABLE, 'o')
            ->condition('o.parent_id',$pid->aid, '=')
            ->fields('o')
            ->orderBy('o.weight', 'ASC')
            ->execute();

    while ($node = $third->fetchObject()) {
      $thirdlevel[$node->aid]=$node;
      $top = $secondtofirst[$node->parent_id];
      $root[$top]['children'][$node->parent_id]['children'][$node->aid] = $node;
    }
  }

    $form['title'] = array(
          '#type'=>'markup',
          '#value'=>
            '<table><tr><td>Value</td><td>Text</td><td>Weight</td><td>Enable/Disable</td><td>Default</td><td>Modify</td></tr></table>',
    );
    // header for first level
    $form['first'] = array(
          '#type'=> 'fieldset',
          '#title'=> "level 1 options",
          '#collapsible'=>true,
          '#collapsed'=>false,
          '#states' => array('invisible' => $invisible_state)
    );

    foreach ($root as $first) {

      //data for first level
      cvwocodem2_fill_options($form['first'], $form_state, $first['data']);

      if(!empty($first['children'])) {

        //header for second level
        $node1= $first['data'];
        $form['first']['children_'.$node1->aid] = array(
          '#type'=> 'fieldset',
          '#title'=> "Suboptions for ".$node1->ansval,
          '#collapsible'=>true,
          '#collapsed'=>false,
        );

        foreach($first['children'] as $second){
          //data for second level
          cvwocodem2_fill_options($form['first']['children_'.$node1->aid], $form_state, $second['data']);

          if(!empty($second['children'])) {

            //header for third level
            $node2 = $second['data'];
            $form['first']['children_'.$node1->aid]['children_'.$node2->aid] = array(
              '#type'=> 'fieldset',
              '#title'=> "Suboptions for ".$node2->ansval,
              '#collapsible'=>true,
              '#collapsed'=>false,
              );

            foreach($second['children'] as $third){
              //data for third level
              cvwocodem2_fill_options($form['first']['children_'.$node1->aid]['children_'.$node2->aid], $form_state, $third);

            }
               //add option for third level
              cvwocodem2_add_options($form['first']['children_'.$node1->aid]['children_'.$node2->aid],
                   $values, $node2->aid, 'Add New Option');


          }else if(!empty($thirdlevel)){
            //add option for second level
              cvwocodem2_add_options($form['first']['children_'.$node1->aid],
                   $values, $second['data']->aid, 'Add Child Option');

          }

        }


      }else if(!empty($secondlevel)){
          cvwocodem2_add_options($form['first'], $values, $first['data']->aid, 'Add Child Option');

      }

    }
      cvwocodem2_add_options($form['first'], $values, -1, 'Add New Option');



    $form['dabug'] = array(
      '#type'=>'markup',
      '#value'=> '<pre>'.print_r($options, true). '</pre>',
    );
    return $form;

}

function cvwocodem2_add_options(&$form, $values, $parent_id, $title) {
      if ($parent_id != -1 && !isset($values['newval_'.$parent_id])) return;

        /*
         * Add new option
         */
      $form['add_'.$parent_id] = array(
        '#type'=> 'fieldset',
        '#title'=> $title,
        '#collapsible'=> true,
        '#collapsed' => true,
      );

      $form['add_'.$parent_id]['newtext_'.$parent_id] = array(
        '#type' => 'textfield',
        '#name' => 'newtext_'.$parent_id,
        '#title'=>t('Text'),
        '#value' => isset($values['newtext_'.$parent_id]) ? $values['newtext_'.$parent_id] : '',
        '#prefix' => '<table><tr><td>',
        '#suffix' => '</td>',
        '#size' => 20,
        '#maxlength' => 255,
      );

      $form['add_'.$parent_id]['newval_'.$parent_id] = array(
        '#type' => 'textfield',
        '#name' => 'newval_'.$parent_id,
        '#title'=> t('Value'),
        '#value' => isset($values['newval_'.$parent_id]) ? $values['newval_'.$parent_id] : '',
        '#prefix' => '<td>',
        '#suffix' => '</td>',
        '#size' => 20,
        '#maxlength' => 255,
      );

      $form['add_'.$parent_id]['newweight_'.$parent_id] = array(
        '#type' => 'textfield',
        '#name' => 'newweight_'.$parent_id,
        '#default_value' => '0',
        //'#value'=>$values['newweight_'].$parent_id,
        '#title' => t('Weight'),
        '#prefix' => '<td>',
        '#suffix' => '</td>',
        '#size' => 20,
        '#maxlength' => 255,
      );

      $form['add_'.$parent_id]['addnew_'.$parent_id] = array(
        '#type' => 'submit',
        '#name' => 'addnew_'.$parent_id,
        '#value' => t('Add Option'),
        '#prefix' => '<td>',
          '#suffix' => '</td><td><span style="color:red">Cannot Undo!</span></td></tr></table>',
      );

}

function cvwocodem2_fill_options(&$form, &$form_state, $node) {

        $form['ansval_'.$node->aid] = array(
          '#type' => 'item',
          '#value'=>$node->ansval,
          '#prefix'=>'<table><tr><td><b>',
          '#suffix'=>'</b></td>',
        );

        $form['anstext_'.$node->aid] = array(
          '#type' => 'textfield',
          '#value' => $node->anstext,
          '#prefix' => '<td>',
          '#suffix' => '</td>',
          '#size' => 15,
          '#maxlength' => 255,
        );


        $form['weight_'.$node->aid] = array(
          '#type'=>'textfield',
          '#value'=>$node->weight,
          '#size'=>5,
          '#prefix'=>'<td>',
          '#suffix'=>'</td>'
        );

        /*
         * Show disabled status and button
         */

        $form['toggle_'.$node->aid] = array(
          '#type' => 'submit',
          '#name' => 'toggle_'.$node->aid,
          '#value' => t('Toggle'),
          '#prefix' => '<td>'.($node->disabled == 1 ? 'Disabled' : 'Enabled'),
          '#suffix' => '</td>',
        );
        $form['defaultopt_'.$node->aid] = array(
          '#type' => 'submit',
          '#name' => ($node->defaultopt == 1? 'undefault_'.$node->aid: 'defaultopt_'.$node->aid),
          '#value' => ($node->defaultopt == 1? t('Unset') : t('Set Default')),
          '#prefix' => '<td>'.($node->defaultopt == 1 ? 'Yes' : ''),
          '#suffix' => '</td>',
        );
        $form['modify_'.$node->aid] = array(
          '#type' => 'submit',
          '#name' => 'modify_'.$node->aid,
          '#value' => t('Modify'),
          '#prefix' => '<td>',
          '#suffix' => '</td></tr></table>',
        );



        $form['pid_'.$node->aid] = array(
          '#type'=> 'hidden',
          '#value'=> $node->parent_id,
        );



}


function cvwocodem2_form_validate($form, &$form_state) {
  if (empty($form_state['clicked_button'])) {
    return;
  }
  return;

  $button = $form_state['clicked_button']['#name'];
  $vals=$form_state['clicked_button']['#post'];
  if(substr($button, 0, 6)== 'addnew') {

    list($op, $pid) = explode("_", $button);
      if(!is_numeric($vals['newweight_'.$pid]))
        form_set_error('newweight_'.$pid,'Please enter a number for weight');
      if(empty($vals['newval_'.$pid]))
        form_set_error('newval_'.$pid,'Please enter the new value');
      if(empty($vals['newtext_'.$pid]))
        form_set_error('newtext_'.$pid,'Please enter the new text');

  }
  if(substr($button,0,6) == 'modify') {
    list($op, $aid) = explode("_",$button);
    if(!is_numeric($vals['weight_'.$aid]))
        form_set_error('weight_'.$aid,'Please enter a number for weight');
    if(is_null($vals['anstext_'.$aid]))
        form_set_error('anstext_'.$aid,'New option text cannot be empty');

  }
}

function cvwocodem2_form_submit($form, &$form_state){
  $button = substr($form_state['clicked_button']['#name'],0,6);
  if ($button != 'hidden') {
    $vals=$form_state['input'];
    list($op, $pid) = explode("_", $form_state['clicked_button']['#name']);
  }
  switch($button) {
    case 'addnew':
      if(cvwocodem2_addoption(
          $vals['qid'],$vals['newval_'.$pid], $vals['newtext_'.$pid],$vals['newweight_'.$pid], $pid))
       drupal_set_message("The new option has been added.");
      break;
    case 'modify':
      cvwocodem2_modifyoption($pid, $vals['anstext_'.$pid], $vals['weight_'.$pid], $vals['pid_'.$pid]);
      break;
    case 'defaul':
      cvwocodem2_defaultopt($pid, $vals['qid'], $vals['pid_'.$pid]);
      break;
    case 'toggle':
      cvwocodem2_toggleoption($pid);
      drupal_set_message("Option has been updated.");
      break;
    case 'qtitle':
      if(!empty($vals['qtitle']) && cvwocodem2_updatetitle($pid, $vals['qtitle'])) {
        drupal_set_message("Question Title Updated");
      }
      break;
    case 'undefa':
      cvwocodem2_undefaultopt($pid);
      break;
    case 'hidden':
      $form_state['hiddentrigger'] = 1;
      break;
  }

  $form_state['rebuild'] = true;

}