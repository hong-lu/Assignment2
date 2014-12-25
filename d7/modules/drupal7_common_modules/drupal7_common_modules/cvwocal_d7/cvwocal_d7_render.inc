<?php
/*
  Copyright (c) 2011-2012 Computing for Volunteer Welfare Organisations (CVWO)
  National University of Singapore
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
  files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:
   
  1. The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
  Software.
   
  2. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
// $Id$

function cvwocal_render($form, &$form_state, $year = NULL, $month = NULL, $day = NULL, $person_id = NULL, $centre_id = NULL, $location_id = NULL) {
  global $user;
  
  $form['#attached']['library'][] = array('system', 'ui.draggable');
  $form['#attached']['library'][] = array('system', 'ui.resizable');
  $form['#attached']['library'][] = array('system', 'ui.dialog');
  $form['#attached']['library'][] = array(CVWOBASE_MODULE, 'qTip');
  $form['#attached']['css'][] = drupal_get_path('module', CVWOCAL_MODULE).'/fullcalendar/fullcalendar.css';
  $form['#attached']['js'][] = drupal_get_path('module', CVWOCAL_MODULE).'/fullcalendar/fullcalendar.min.js';
  $form['#attached']['js'][] = drupal_get_path('module', CVWOCAL_MODULE).'/fullcal_loader.js';

  // $person_id = isset($form_state['values']['person_id']) ? $form_state['values']['person_id'] : '';
  // $centre_id = isset($form_state['values']['centre_id']) ? $form_state['values']['centre_id'] : '';
  // $location_id = isset($form_state['values']['location_id']) ? $form_state['values']['location_id'] : '';
  
  $settings = array(
    'id' => CVWOCAL_CAL_ID,
    'event_source' => CVWOCAL_MODULE.'/events/' . $person_id . '/' . $centre_id . '/' . $location_id,
    'edit_url' => CVWOCAL_MODULE.'/ajaxedit',
    'drop_tag' => CVWOCAL_CHANGE_TIMING,
    'resize_tag' => CVWOCAL_CHANGE_DURATION,
    'error_tag' => CVWOCAL_ERROR_TAG,
    'empty_event' => '',
    'event_type_selector' => '#edit-event-options'
 );
  
  if (@checkdate($month, $day, $year)) {
    $settings['year'] = $year;
    $settings['month'] = $month;
    $settings['day'] = $day;
  }
  
 	$form['filter'] = array(
		'#type' => 'fieldset',
		'#collapsible' => FALSE,
		'#theme' => 'cvwocal_filter',
		'#weight' => -1,
    '#tree' => TRUE
	);
 
  $args = &$form_state['build_info']['args'];
  
  $query = cvwo_select(CVWO_PERSON_TABLE, 'p')->fields('p', array('person_id'))->addTag('cvwocal_filter')->addTag('cvwocal_person');
	$query->addExpression('CONCAT(first_name, \' \', last_name)', 'name');
  $person_options = $query->orderBy('p.first_name')->execute()->fetchAllKeyed();
  
	$form['filter']['person_id'] = array(
		'#type' => 'select',
		'#title' => t('Person Involved'),
		'#empty_option' => t('All'),
		'#options' => $person_options,
		'#default_value' => empty($args[3]) ? $person_id : $args[3]
	);
  
  $query = cvwo_select(CVWO_CENTRE_TABLE, 'c')->fields('c', array('centre_id', 'centre_name'))->addTag('cvwocal_filter')->addTag('cvwocal_centre');
	$centre_options = $query->orderBy('centre_name')->execute()->fetchAllKeyed();
	
	$form['filter']['centre_id'] = array(
		'#type' => 'select',
		'#title' => t('Centre'),
		'#empty_option' => t('All Centres'),
		'#options' => $centre_options,
    '#default_value' => empty($args[4]) ? '' : $args[4]
	);
	
  $query = cvwo_select(CVWO_LOCATION_TABLE, 'l')->fields('l', array('location_id', 'location_name'))->addTag('cvwocal_filter')->addTag('cvwocal_location');
	$location_options = $query->orderBy('location_name')->execute()->fetchAllKeyed();
	
	$form['filter']['location_id'] = array(
		'#type' => 'select',
		'#title' => t('Location'),
		'#empty_option' => t('All Locations'),
		'#options' => $location_options,
    '#default_value' => empty($args[5]) ? '' : $args[5]
	);
  
  $form['filter']['filter_btn'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#submit' => array('cvwocal_filter_submit'),
    '#limit_validation_errors' => array(array('filter'))
  );
  
  $form['#attached']['js'][] = array(
    'data' => array('fullcalendar_settings' => $settings),
    'type' => 'setting'
  );

  $form['cal_container'] = array(
    '#theme' => 'container',
    '#attributes' => array(
      'id' => CVWOCAL_CAL_ID
    )
  );
 
  if (!($types = cvwocal_get_types(TRUE))) {
    return $form;
  } 
  $form['event_form'] = array(
    '#type' => 'container',
    '#title' => 'event_form',
    '#attributes' => array(
      'id' => "event_form",
      'title' => t('Select an Event Type'),
    )
  );
  
  $form['event_form']['event_options'] = array(
    '#type' => 'select',
    '#title' => 'Event type',
    '#options' => array_combine(array_map(create_function('$e', 'return $e["edit_url"];'), $types), array_map(create_function('$e', 'return $e["name"];'), $types)),
    '#required' => TRUE,
    '#empty_option' => t('Please Select')
  );
  
  return $form;
}

function cvwocal_filter_submit($form, &$form_state) {
  $date_args = implode('/', array_slice($form_state['build_info']['args'], 0, 3) + array('', '', ''));
  $vals = &$form_state['values']['filter'];
  $form_state['redirect'] = CVWOCAL_MODULE.'/'.$date_args.'/'.$vals['person_id'].'/'.$vals['centre_id'].'/'.$vals['location_id'];
}

function theme_cvwocal_filter($variables){
	$element = $variables['form'];
	
	$output = '<table>';
	$output .= '<tr>';
	$output .= '<td>' . drupal_render($element['person_id']) . '</td>';
	$output .= '<td>' . drupal_render($element['centre_id']) . '</td>';
	$output .= '<td>' . drupal_render($element['location_id']) . '</td>';
	$output .= '</tr>';
	$output .= '</table>';
	$output .= drupal_render_children($element);
	
	return $output;
}



function cvwocal_ajaxedit($type, $module, $event_id, $change_type, $day_delta, $minute_delta, $all_day = NULL) {
  $allowed_types = cvwocal_get_types(TRUE);
  
  // callback functions might expect certain types
  $args = array_slice(func_get_args(), 1);
  settype($args[3], 'int');
  settype($args[4], 'int');
  if (isset($args[5]))
    settype($args[5], 'bool');
  
  if (empty($allowed_types[$type]['edit_callback']) || !function_exists($allowed_types[$type]['edit_callback']) || !call_user_func_array($allowed_types[$type]['edit_callback'], $args))
    return array(CVWOCAL_ERROR_TAG => TRUE);
  return array();
}

function cvwocal_events($person_id = NULL, $centre_id = NULL, $location_id = NULL) {
  if (!isset($_GET['start'], $_GET['end']))
    return array();
  $start = $_GET['start'];
  $end = $_GET['end'];
  $allowed_types = cvwocal_get_types(TRUE); 

  $events = array();
  foreach (module_implements('cal_events') as $module)
    if (function_exists($f = $module.'_cal_events') && is_array($result = $f($start, $end, $person_id, $centre_id, $location_id)))
      foreach ($result as $id => &$event) {
        $event['module'] = $module;
        $event['internal_id'] = $id;
        $event['editable'] = (isset($event['editable']) ? $event['editable'] : TRUE) && (!empty($allowed_types[$type = $event['type']]['edit_callback']) && function_exists($allowed_types[$type]['edit_callback']));
        if (!empty($event['repeat_id']))
          $event['id'] = $module.$event['repeat_id'];
        else
          $event['id'] = $module.$id;
        
        $content[t('Title')] = check_plain($event['title']);
        if (!empty($event['description']))
          $content[t('Description')] = is_string($event['description']) ? filter_xss($event['description']) : drupal_render($event['description']);
        $event['qtip_content'] = theme('table', array(
          'header' => array(),
          'rows' => array_map(create_function('$title, $content', 'return array(array("data" => $title, "header" => TRUE), $content);'), array_keys($content), $content)
        ));
        $events[] = $event;
      }
  return $events;
}

function cvwocal_edit($form, &$form_state, $type = '', $event_id = 0, $start = '', $end = '') {
  $allowed_types = cvwocal_get_types(TRUE);
  
  // we store the type information inside here
  $form_state[CVWOCAL_MODULE]['type'] = $type;
  
  if (isset($allowed_types[$type]) && isset($allowed_types[$type]['form_callback']))
    if (function_exists($func = $allowed_types[$type]['form_callback']))
      $form = $func($form, $form_state, $event_id, $start, $end);
  return $form;
}

function cvwocal_edit_validate($form, &$form_state) {
  $allowed_types = cvwocal_get_types(TRUE);
  $type = $form_state[CVWOCAL_MODULE]['type'];
  
  if (isset($allowed_types[$type]) && isset($allowed_types[$type]['form_callback']))
    if (function_exists($func = $allowed_types[$type]['form_callback'].'_validate'))
      $func($form, $form_state);
}

function cvwocal_edit_submit($form, &$form_state) {
  $allowed_types = cvwocal_get_types(TRUE);
  $type = $form_state[CVWOCAL_MODULE]['type'];
  
  if (isset($allowed_types[$type]) && isset($allowed_types[$type]['form_callback']))
    if (function_exists($func = $allowed_types[$type]['form_callback'].'_submit'))
      $func($form, $form_state);
}