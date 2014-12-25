<?php
require_once drupal_get_path('module', CVWODATAVALIDATE_MODULE).'/cvwodatavalidate_api.inc';

function lions_data_validate($form, &$form_state, $nric_colour = null) {
	$form = array();

/****************************
This is the frist query

This query is used to build the region selections for users
****************************/

	$query = cvwo_select(CVWO_PERSON_TABLE,'p');
	$query    ->join(CVWO_PERSON_CONTACT_TABLE, 't', 'p.person_id = t.person_id');
	$query    ->join(CVWO_PERSON_LANGUAGE_TABLE, 'l', 'l.person_id = p.person_id');
	//$query    ->join(CVWO_PERSON_NOK_TABLE, 'n', 'n.person_id = p.person_id');
    $query	  ->addExpression('COUNT(p.person_id)', 'number');
    //remember the first query only do the group
    //so it only needs the one field
    //for example we can group by nric_color
    $query    ->fields('p', array('nric_colour'));
	$query	  ->condition(db_or()
				          ->condition('p.nric_fin', '')
					      ->condition('p.first_name', '')
					      ->condition('p.last_name', '')
					      ->condition('p.gender', '')
					      ->condition('p.date_of_birth', '')
					      ->condition('p.race', '')
					      ->condition('p.religion', '')
	                      )
			  ->condition(db_or()
			             ->condition('t.mobile', null)
				         ->condition('l.language', null)
				         )
			  ->condition('p.nric_colour', '', '<>');
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>cvwo_validate_start
//you can add more conditions here 
//conditions about how to define the data is validate or not
//don't forget also change in the second query

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>cvwo_validate_end

	// you cound change this line to modify by which column you want to group
	$query    ->groupBy('p.nric_colour');

	$result = $query->execute();
	$total = 0;
	$stats = '';

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>cvwo_validate_end
	while($data = $result->fetchObject()) {
    // you could change the message to show by changint these loop
		$stats.= '    '.$data->nric_colour . ' - ';
		$stats.= l($data->number, CVWO_DATA_VALIDATE_NAME. "/" .$data->nric_colour, array());

		//this check case for $total is for self implemented pager
		//can comment this out if you are using the default one
		if(!isset($region))
		    $total += $data->number;
		else
		{
			if($data->nric_colour==$nric_colour)
			    $total = $data->number;
		}
	}

	$form['nric_colour'] = array(
		'#type'=>'item',
		'#markup'=> $stats,
	);
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>cvwo_validate_end

/*****************************
This is the second query

This is the query which really access the form and displays the entries
******************************/

	$query2 = cvwo_select(CVWO_PERSON_TABLE,'p');
	$query2    ->join(CVWO_PERSON_CONTACT_TABLE, 't', 'p.person_id = t.person_id');
	$query2    ->join(CVWO_PERSON_LANGUAGE_TABLE, 'l', 'l.person_id = p.person_id');
	$query2    ->join(CVWO_PERSON_NOK_TABLE, 'n', 'n.person_id = p.person_id');


	$query2    ->fields('p');
	$query2    ->fields('t', array('mobile'));
	$query2    ->fields('b', array('language'));

	$query2	  ->addExpression('COUNT(p.person_id)', 'number');

	$query2	  ->condition(db_or()
				          ->condition('p.nric_fin', '')
					      ->condition('p.first_name', '')
					      ->condition('p.last_name', '')
					      ->condition('p.gender', '')
					      ->condition('p.date_of_birth', '')
					      ->condition('p.race', '')
					      ->condition('p.religion', '')
	                      )
			  ->condition(db_or()
			             ->condition('t.mobile', null)
				         ->condition('l.language', null)
				         );
	if(strlen($nric_colour) > 0){
	    $query2    ->condition('p.nric_colour', $region);
    }
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>cvwo_validate_start
//you can add more conditions here 
//conditions about how to define the data is validate or not
//don't forget also change in the first query

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>cvwo_validate_end
    //this means do not group, OR group by each person
    $query2->groupBy('p.person_id');
    //and this query implements the default pager
    $query2 = $query2 -> extend('PagerDefault')
                      -> limit(15);

	$result2 = $query2->execute();

	while($data2 = $result2->fetchObject()) {
	    modify_form($form, $data2);
	}

	//this change the theme of the form to 'pager'
	$form['pager'] = array('#markup' => theme('pager'));

	return $form;
}

//this method builds the form
function modify_form(&$form, $data)
{

    $form[''.$data->person_id] = array(
        '#type' =>'fieldset',
        '#title' => t(''.$data->english_name.'   '.$data->nric_fin),
        '#collapsible' => TRUE,
        '#collapsed' =>TRUE,
    );

	$form[''.$data->person_id]['nric'.$data->person_id]= array(
	    '#type' => 'textfield',
		'#size' => '10',
		'#default_value' => $data->nric_fin,
		'#title' => t('NRIC/FIN'),
		'#name'=>'nric'.$data->person_id,
		'#prefix' =>'<table><tr><td>',
		'#suffix' => '</td>'
	);

	$options = cvwocodem2_getoptions(LIONS_BASE_MODULE_NAME, LIONS_BASE_RACE_QUESTION, 0, false);
	$options['data_validate_nothing'] = t('No Value');
	$form[''.$data->person_id]['race'.$data->person_id]= array(
		'#type' => 'radios',
		'#name'=>'race'.$data->person_id,
		'#default_value' => array_key_exists($data->race, $options)?$data->race:'data_validate_nothing',
		'#title' => t('Race'),
		'#options' => $options,//array('Chinese' => t('Chinese'), 'Indian' => t('Indian'),'Malay' => t('Malay'),'European' => t('European'),'Eurasian' => t('Eurasian'),'Others' => t('Others')),
		//'#disabled' => $race,
		'#prefix' =>'<th rowspan = 4>',
		'#suffix' => '</th>'
	);

	$options = cvwocodem2_getoptions(LIONS_BASE_MODULE_NAME, LIONS_BASE_RELIGION_QUESTION,0, false);
	$options['data_validate_nothing'] = t('No Value');
	$form[''.$data->person_id]['religion'.$data->person_id]= array(
		'#type' => 'radios',
		'#name'=>'religion'.$data->person_id,
		'#default_value' => array_key_exists($data->religion, $options)?$data->religion:'data_validate_nothing',
		'#title' => t('Religion'),
		'#options' => $options,//array('Buddhism' => t('Buddhism'), 'Christianity' => t('Christianity'),'Hinduism' => t('Hinduism'),'Islam' => t('Islam'),'Sikhism' => t('Sikhism'),'Taoism' => t('Taoism'),'Catholicism' => t('Catholicism'),'No Religion' => t('No Religion'),'Others'=>t('Others')),
		//'#disabled' => $religion,
		'#prefix' =>'<th rowspan = 4>',
		'#suffix' => '</th>'
	);

	$form[''.$data->person_id]['save'.$data->person_id]= array(
		'#type' => 'submit',
		'#name'=>'save'.$data->person_id,
		'#value' => t('Save'),

		'#prefix' =>'<th rowspan = 4>',
		'#suffix' => '</th></tr>'
	);

	$form[''.$data->person_id]['first_name'.$data->person_id]= array(
		'#type' => 'textfield',
		'#size' => '40',
		'#default_value' => $data->first_name,
		'#name'=>'first_name'.$data->person_id,
		'#title' => t('First Name'),
		'#prefix' =>'<tr><td>',
		'#suffix' => '</td></tr>'
	);

	$form[''.$data->person_id]['last_name'.$data->person_id]= array(
		'#type' => 'textfield',
		'#size' => '40',
		'#default_value' => $data->last_name,
		'#name'=>'last_name'.$data->person_id,
		'#title' => t('Last Name'),
		'#prefix' =>'<tr><td>',
		'#suffix' => '</td></tr>'
	);

	$options = cvwocodem2_getoptions(LIONS_BASE_MODULE_NAME, LIONS_BASE_GENDER_QUESTION, 0, false);
	$options['data_validate_nothing'] = t('No Value');
	$form[''.$data->person_id]['gender'.$data->person_id] = array(
		'#type'=>'radios',
		'#name'=>'gender'.$data->person_id,
		'#default_value' => array_key_exists($data->gender, $options)?$data->gender:'data_validate_nothing',
		'#title' => t('Gender'),
		'#options' => $options, //array('Female' => t('Female'), 'Male' => t('Male')),
		'#prefix' =>'<tr><td>',
	    '#suffix' => '</td></tr>'

	);

	$form[''.$data->person_id]['date_of_birth'.$data->person_id]= array(
		'#type' => 'date_popup',
		'#name'=>'date_of_birth'.$data->person_id,
		'#default_value' => $data->date_of_birth,
		'#title' => t('Date of Birth'),
		'#date_format' => 'd-m-Y',
		'#prefix' =>'<tr><td>',
		'#suffix' => '</td></tr>'
	);

	$form[''.$data->person_id]['last_name'.$data->person_id]= array(
		'#type' => 'textfield',
		'#size' => '40',
		'#default_value' => $data->mobile,
		'#name'=>'last_name'.$data->person_id,
		'#title' => t('Mobile Number'),
		'#prefix' =>'<tr><td>',
		'#suffix' => '</td></tr>'
	);

	$form[''.$data->person_id]['last_name'.$data->person_id]= array(
		'#type' => 'textfield',
		'#size' => '40',
		'#default_value' => $data->language,
		'#name'=>'last_name'.$data->person_id,
		'#title' => t('Language'),
		'#prefix' =>'<tr><td>',
		'#suffix' => '</td></tr>'
	);
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>cvwo_validate_start
//this is where you can add more fields you want to display

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>cvwo_validate_end

/*
//This is a nice example to show the address
//it is a pity it does not fit cvwo_person
//pls custimize this part

	$store = array();
	$address = 'Address: <br>';
	$address .= lions_base_display_address($data->person_id);
	$address .= l(t('Edit Address'), LIONS_BEFRIENDEE_MODULE_NAME. "/edit/" .$data->person_id.'#');

	$form[''.$data->person_id]['address'.$data->person_id]= array(
		'#title' => t('Address'),
		'#markup' => $address,
		'#prefix' =>'<tr><td>',
		'#suffix' => '</td></tr></table>'
	);
*/

/*
//another example if you want to pass any values to the $form_state['values'], the proper way is to do it here like this

	$form[''.$data->person_id]['person_id'.$data->person_id]= array(
		'#type' => 'value',
		'#name'=>'person_id'.$data->person_id,
		'#value' => $data->person_id,
	);

	$form[''.$data->person_id]['person_type'.$data->person_id]= array(
		'#type' => 'value',
		'#name'=>'person_type'.$data->person_id,
		'#value' => $data->person_type,
	);
*/
 
	return $form[''.$data->person_id];
}

function lions_data_validate_submit(&$form ,&$form_state)
{

	$values = array();
	$button = $form_state['clicked_button']['#name'];
	$person_id = substr($button,4);

	$values['nric'.$person_id] = $form_state['values']['nric'.$person_id];
	$values['first_name'.$person_id]= $form_state['values']['first_name'.$person_id];
	$values['last_name'.$person_id]= $form_state['values']['last_name'.$person_id];
	$values['gender'.$person_id] = $form_state['values']['gender'.$person_id]=='data_validate_nothing'?'':$form_state['values']['gender'.$person_id];
	$values['date_of_birth'.$person_id] = $form_state['values']['date_of_birth'.$person_id];
    $values['race'.$person_id] = $form_state['values']['race'.$person_id]=='data_validate_nothing'?'':$form_state['values']['race'.$person_id];
	$values['religion'.$person_id]  = $form_state['values']['religion'.$person_id]=='data_validate_nothing'?'':$form_state['values']['religion'.$person_id];
	$values['mobile'.$person_id]= $form_state['values']['mobile'.$person_id];
	$values['language'.$person_id]= $form_state['values']['language'.$person_id];

	lions_data_validate_update($values,$person_id);
	drupal_set_message("Record is complete and saved sucessfully.");
}
