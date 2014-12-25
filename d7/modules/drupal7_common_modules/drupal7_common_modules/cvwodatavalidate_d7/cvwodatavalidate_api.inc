<?php
/**
 * Copyright (c) 2010-2013
 * Computing for Volunteer Welfare Organizations (CVWO)
 * National University of Singapore
 *
 * Permission is hereby granted, free of charge, to any person obtaining
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
 */
function cvwo_data_validate_update($values, $person_id)
{
	global $user;
	$username = $user->name;	

	$log_message = $username . " update person record for " .$person_id;

	$id = cvwo_update(CVWO_PERSON_TABLE);
	$id ->fields(array(
        'nric_fin' => $values['nric'.$person_id],
	    'first_name' => $values['first_name'.$person_id],
	    'last_name' => $values['last_name'.$person_id],
		'gender' => $values['gender'.$person_id],
		'date_of_birth' => $values['date_of_birth'.$person_id],
		'race' => $values['race'.$person_id],
		'religion' => $values['religion'.$person_id],
		'date_modified' => date('Y-m-d H:i:s',time()),
		'user_modified' => $username,
	));
	$id ->condition('person_id', $person_id, '=');
	$id ->execute_logged($log_message, 'edit', CVWO_DATA_VALIDATE_NAME);

	$id2 = cvwo_update(CVWO_PERSON_CONTACT);
	$id2 -> fileds(array(
		'mobile' => $values['mobile'.$person_id],
	));
	$id2 ->condition('person_id', $person_id, '=');
	$id2 ->execute_logged($log_message, 'edit', CVWO_DATA_VALIDATE_NAME);

	$id3 = cvwo_update(CVWO_PERSON_LANGUAGE);
	$id3 -> fields(array(
		'language' => $values['language'.$person_id],
	));
	$id3 ->condition('person_id', $person_id, '=');
	$id3 ->execute_logged($log_message, 'edit', CVWO_DATA_VALIDATE_NAME);

/*
	if($values['person_type'.$person_id]==3)
	{
    	$username = $user->name;

	    $log_message = $username . " update befriendee record for " .$person_id;

        $id = cvwo_update(LIONS_BEFRIENDEE_TABLE);
	    $id ->fields(array(
		    'at_risk' => $values['at_risk'.$person_id],
		    'classification' => $values['classification'.$person_id],
		    'service_boundary' => $values['service_boundary'.$person_id],
		    'date_modified' => date('Y-m-d H:i:s',time()),
		    'staff_modified' => $username,
		    ));
		$id ->condition('personid', $person_id, '=');
		$id ->execute_logged($log_message, 'edit', CVWO_DATA_VALIDATE_NAME);
	}
*/
	return $id;
}
