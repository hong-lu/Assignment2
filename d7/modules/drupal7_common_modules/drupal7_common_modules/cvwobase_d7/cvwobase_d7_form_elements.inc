<?php
/*
 * The MIT License
 *
 * Copyright 2011-2013 Computing for Volunteer Welfare Organizations (CVWO),
 * National University of Singapore.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/**
 * @file
 * Custom page elements for cvwobase_d7.
 */

namespace CVWO\Base {

/**
 * Process function to expand the files element type.
 *
 * Expands the file type to include Upload and Remove buttons, as well as
 * support for a default value.
 */
function cvwobase_files_process($element, &$form_state, $form) {
  $fids = isset($element['#value']) ? $element['#value'] : array();

  // Set some default element properties.
  $element['#progress_indicator'] = empty($element['#progress_indicator']) ? 'none' : $element['#progress_indicator'];
  $element['#files'] = file_load_multiple($fids);
  $element['#tree'] = TRUE;

  $ajax_settings = array(
    'path' => 'file/ajax/' . implode('/', $element['#array_parents']) . '/' . $form['form_build_id']['#value'],
    'wrapper' => $element['#id'] . '-ajax-wrapper',
    'callback' => 'cvwobase_files_ajax_callback'
  );

  $element['fids'] = array(
    '#type' => 'hidden',
    '#value' => $fids,
  );
    
  if (empty($element['#disabled'])) {
    // The file upload field itself.
    $element['upload'] = array(
      '#name' => 'files[' . implode('_', $element['#parents']) . ']',
      '#type' => 'file',
      '#title' => t('Choose a file'),
      '#title_display' => 'invisible',
      '#size' => 22,
      '#theme_wrappers' => array()
    );
    $element['upload_button'] = array(
      '#type' => 'submit',
      '#value' => t('Upload'),
      '#validate' => array(),
      '#submit' => array('\CVWO\Base\cvwobase_files_submit'),
      '#limit_validation_errors' => array($element['#parents']),
      '#ajax' => $ajax_settings
    );
    
    // Add progress bar support to the upload if possible.
    if ($element['#progress_indicator'] == 'bar' && $implementation = file_progress_implementation()) {
      $upload_progress_key = mt_rand();
  
      if ($implementation == 'uploadprogress')
        $element['UPLOAD_IDENTIFIER'] = array(
          '#type' => 'hidden',
          '#value' => $upload_progress_key,
          '#attributes' => array('class' => array('file-progress')),
        );
      elseif ($implementation == 'apc')
        $element['APC_UPLOAD_PROGRESS'] = array(
          '#type' => 'hidden',
          '#value' => $upload_progress_key,
          '#attributes' => array('class' => array('file-progress')),
        );
        
      // Add the upload progress callback.
      $element['upload_button']['#ajax']['progress']['path'] = 'file/progress/' . $upload_progress_key;
    }
  }

  $element['old']['#theme'] = 'cvwobase_files_old';
  $ajax_settings['progress']['message'] = NULL;
  $ajax_settings['effect'] = 'none';
  foreach($element['#files'] as $fid => $file) {
    $element['old'][$fid]['filename'] = array(
      '#theme' => 'file_link',
      '#file' => $file
    );
    if (empty($element['#disabled']))
      $element['old'][$fid]['remove_button'] = array(
        '#type' => 'submit',
        '#value' => t('Remove'),
        '#validate' => array(),
        '#submit' => array('\CVWO\Base\cvwobase_files_submit'),
        '#limit_validation_errors' => array($element['#parents']),
        '#ajax' => $ajax_settings
      );
  }

  // Add the extension list to the page as JavaScript settings.
  if (isset($element['#upload_validators']['file_validate_extensions'][0])) {
    $extension_list = implode(',', array_filter(explode(' ', $element['#upload_validators']['file_validate_extensions'][0])));
    $element['upload']['#attached']['js'] = array(
      array(
        'type' => 'setting',
        'data' => array('file' => array('elements' => array('#' . $element['#id'] . '-upload' => $extension_list)))
      )
    );
  }

  // Prefix and suffix used for Ajax replacement.
  $element['#prefix'] = '<div id="' . $element['#id'] . '-ajax-wrapper">';
  $element['#suffix'] = '</div>';
  return $element;
}

/**
 * The #value_callback for a files type element.
 */
function cvwobase_files_value(&$element, $input = FALSE, $form_state = NULL) {
  $fids = array();

  // Find the current value of this field from the form state.
  $form_state_fids = drupal_array_get_nested_value($form_state['values'], $element['#parents']);
  if (!$form_state_fids)
    $fids = array();
  elseif (is_array($form_state_fids))
    $fids = array_filter($form_state_fids, 'is_numeric');

  // Process any input and save new uploads.
  if ($input !== FALSE) {
    if (isset($input['fids']) && $files = file_load_multiple(explode(' ', $input['fids'])))
      $fids = array_keys($files);
    // Upload any file that was left in the file field without clicking submit
    if ($file = file_managed_file_save_upload($element))
      $fids[] = (int) $file->fid;
  }

  // If there is no input, set the default value.
  else {
    $default_fids = isset($element['#default_value']) ? (array) $element['#default_value'] : array();

    // Confirm that the file exists when used as a default value.
    if ($default_fids && $files = file_load_multiple($default_fids))
      $fids = array_keys($files);
  }

  return array_unique($fids);
}

/**
 * An #element_validate callback for the files element.
 */
function cvwobase_files_validate(&$element, &$form_state) {
  // If referencing an existing file, only allow if there are existing
  // references. This prevents unmanaged files from being deleted if this
  // item were to be deleted.
  $clicked_button = end($form_state['triggering_element']['#parents']);
  if ($clicked_button != 'remove_button' && !empty($element['fids']['#value'])) {
    if (($files = file_load_multiple($element['fids']['#value'])) && count($files) == count($element['fids']['#value']))
      foreach ($files as $fid => $file) {
        if ($file->status == FILE_STATUS_PERMANENT) {
          $references = file_usage_list($file);
          if (empty($references))
            form_error($element, t('The file !file used in the !name field may not be referenced.', array('!file' => $file->name, '!name' => $element['#title'])));
        }
      }
    else
      form_error($element, format_plural(count($element['fids']['#value']) - count($files), 'A file referenced by the !name field does not exist', '@count files referenced by the !name field do not exist.', array('!name' => $element['#title'])));
  }

  // Check required property based on the FID.
  if ($element['#required'] && empty($element['fids']['#value']) && !in_array($clicked_button, array('upload_button', 'remove_button')))
    form_error($element['upload'], t('!name field is required.', array('!name' => $element['#title'])));

  // Consolidate the array value of this field to a single FID.
  form_set_value($element, $element['fids']['#value'], $form_state);
}

/**
 * Submit handler for upload and remove buttons of files elements.
 */
function cvwobase_files_submit($form, &$form_state) {
  // Determine whether it was the upload or the remove button that was clicked,
  // and set $element to the managed_file element that contains that button.
  $parents = $form_state['triggering_element']['#array_parents'];
  $button_key = array_pop($parents);

  // No action is needed here for the upload button, because all file uploads on
  // the form are processed by file_managed_file_value() regardless of which
  // button was clicked. Action is needed here for the remove button, because we
  // only remove a file in response to its remove button being clicked.
  if ($button_key == 'remove_button') {
    $rmv_fid = array_pop($parents);
    array_pop($parents); // 'old'
    $element = drupal_array_get_nested_value($form, $parents);
    // If it's a temporary file we can safely remove it immediately, otherwise
    // it's up to the implementing module to clean up files that are in use.
    if ($element['#files'][$rmv_fid] && $element['#files'][$rmv_fid]->status == 0) {
      file_delete($element['#files'][$rmv_fid]);
    }
  } elseif ($button_key == 'upload_button') {
    $element = drupal_array_get_nested_value($form, $parents);
    if ($file = file_managed_file_save_upload($element))
      $new_fid = $file->fid;
  }
  
  // Update both $form_state['values'] and $form_state['input'] to reflect
  // the new list of files, so that the form is rebuilt correctly.
  // $form_state['values'] must be updated in case additional submit handlers
  // run, and for form building functions that run during the rebuild, such as
  // when the managed_file element is part of a field widget.
  // $form_state['input'] must be updated so that file_managed_file_value()
  // has correct information during the rebuild.
  $values_element = $element;
  $value = &$values_element['#value'];
  if (isset($rmv_fid) && ($idx = array_search($rmv_fid, $value)) !== FALSE)
    unset($value[$idx]);
  elseif (isset($new_fid))
    $value[] = $new_fid;

  form_set_value($values_element, $value, $form_state);
  drupal_array_set_nested_value($form_state['input'], $values_element['#parents'], $value);

  // Set the form to rebuild so that $form is correctly updated in response to
  // processing the file removal. Since this function did not change $form_state
  // if the upload button was clicked, a rebuild isn't necessary in that
  // situation and setting $form_state['redirect'] to FALSE would suffice.
  // However, we choose to always rebuild, to keep the form processing workflow
  // consistent between the two buttons.
  $form_state['rebuild'] = TRUE;
}
}

namespace {
  function theme_cvwobase_files_old($variables) {
    $elems = &$variables['elems'];
    $fids = element_children($elems);
    if (empty($fids))
      return '';
    $header[] = t('File');
    if ($has_remove = isset($elems[reset($fids)]['remove_button']))
      $header[] = t('Actions');
    foreach ($fids as $fid)
      if ($has_remove)
        $rows[] = array(drupal_render($elems[$fid]['filename']), drupal_render($elems[$fid]['remove_button']));
      else
        $rows[] = array(drupal_render($elems[$fid]['filename']));
    return theme('table', array('header' => $header, 'rows' => $rows));
  }
}
