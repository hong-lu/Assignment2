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

/**
 * @file
 * Hooks provided by the cvwocal_d7 module.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Define types of calendar events.
 *
 * This hook enables modules to register types of events, their forms, and
 * their edit callbacks.
 *
 * @return
 *   An array of types, keyed by a unique (untranslated) name. Each type is
 *   an associative array containing the following key-value pairs:
 *   - "edit_url": Drupal URL for editing this type of event.
 *     URL should be internal, and without a trailing slash.
 *     This URL will be appended with /event_id, if it exists, or / for adding
 *   - "edit_callback": AJAX callback to edit events.
 *     Signature should be $callback($module, $event_id, $change_type, $day_delta, $minute_delta, $all_day = NULL).
 *     The arguments for the callback are as follows:
 *     - $module: Name of the module the event originated from.
 *     - $event_id: The key of the event, as specified by hook_cal_events().
 *     - $change_type: Either CVWOCAL_CHANGE_TIMING or CVWOCAL_CHANGE_DURATION
 *     - $day_delta: Days the entire event (CVWOCAL_CHANGE_TIMING) or the end-date (CVWOCAL_CHANGE_DURATION) was moved.
 *     - $minute_delta: Minutes the entire event (CVWOCAL_CHANGE_TIMING) or the end-date (CVWOCAL_CHANGE_DURATION) was moved.
 *     - $all_day: Boolean indicating all-day status. Only used for CVWOCAL_CHANGE_TIMING.
 *     N.B. If the event is repeated, all instances should be updated.
 *     The callback should return a boolean indicating update success.
 *   - "access_callback": The callback which determines the access to the edit callback. Defaults to user_access.
 *   - "access_arguments": An array of arguments for the access callback.
 */
function hook_cal_types() {
  return array(
    'Tai Chi' =>
      array(
        'edit_url' => 'taichi',
        'edit_callback' => 'taichi_event_edit'
      )
  );
}

/**
 * Returns calendar events within a given time period.
 *
 * This hook enables modules to register types of events, their forms, and
 * their edit callbacks.
 *
 * @param int $start
 *   Starting timestamp of the querying period.
 * @param int $end
 *   Ending timestamp of the querying period.
 * @param int $person_id
 *   Optional. Person ID to filter the events by.
 * @param int $centre_id
 *   Optional. Centre ID to filter the events by.
 * @param int $loc_id
 *   Optional. Location ID to filter the events by. Either Centre ID or
 *   Location ID may be specified, but not both.
 *
 * @return
 *   An array of events. The key for each event that should be a unique
 *   identifier within a given module's events. The event itself is an
 *   associative array with the following key-value pairs:
 *   - "type": Required. Name of the type of event.
 *   - "title": Required. Translated title of the event.
 *   - "start": Required. Timestamp of the start of the event.
 *   - "end": Required. Timestamp of the end of the event.
 *   - "allDay": Optional. Defaults to FALSE.
 *   - "repeat_id": Optional. Repeating events should share the same ID.
 *   - "className": Optional. CSS class or Array of CSS classes for the event's element.
 *   - "url": Optional. Link for the calendar event.
 *   - "editable": Optional. Defaults to TRUE. Also depends on the type's edit_callback.
 */
function hook_cal_events() {
  $query = db_select('events', 'e')
                  ->fields('e', array('event_id', 'start', 'end'));
  $query->addExpression('CONCAT(:t1, :at, e.location)', 'title', array(':t1' => t('Tai Chi'), ':at' => ' @ '));
  $query->addExpression(':type', 'type', 'Tai Chi');
  return $query->execute()->fetchAllAssoc('event_id', PDO::FETCH_ASSOC);
}

/**
 * @} End of "addtogroup hooks".
 */