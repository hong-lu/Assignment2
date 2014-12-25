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
 * Admin page include file for the cvwobase_d7 module.
 */


/**
 * Form builder for the cvwobase_d7 configuration form.
 *
 * @see cvwobase_d7_admin_form_validate()
 * @ingroup forms
 */
function cvwobase_d7_admin_form() {
	$form['mail'] = array(
		'#title' => t('CVWO Email settings'),
		'#description' => t('Settings for the CVWO Email system. Host and Username should correspond to the email address for this Site.'),
		'#type' => 'fieldset',
		'#collapsible' => TRUE,
		'#collapsed' => FALSE
	);
	$form['mail'][CVWOBASE_D7_PHP_BINARY] = array(
		'#type' => 'textfield',
		'#title' => t('Full path to the PHP binary'),
		'#description' => t('Used for batch email sending'),
		'#default_value' => variable_get(CVWOBASE_D7_PHP_BINARY, FALSE)
	);
	$form['mail'][CVWOBASE_D7_MAIL_SEND_ASYNC] = array(
		'#type' => 'checkbox',
		'#title' => t('Send emails in batches'),
		'#description' => t('Batch sending will speed up loading of the site, but delay mail sending a little.'),
		'#default_value' => variable_get(CVWOBASE_D7_MAIL_SEND_ASYNC, CVWOBASE_D7_MAIL_SEND_ASYNC_DEFAULT)
	);
	$form['mail'][CVWOBASE_D7_MAIL_HOST] = array(
		'#type' => 'textfield',
		'#title' => t('SMTP Host for sending email'),
		'#description' => t('Should be the server for your email domain. For example, the Host for example@gmail.com is ssl://smtp.gmail.com'),
		'#default_value' => variable_get(CVWOBASE_D7_MAIL_HOST, CVWOBASE_D7_MAIL_HOST_DEFAULT)
	);
	$form['mail'][CVWOBASE_D7_MAIL_USER] = array(
		'#type' => 'textfield',
		'#title' => t('SMTP Username for sending email'),
		'#description' => t('Should correspond to the site email username. For example, the Username for example@gmail.com is example'),
		'#default_value' => variable_get(CVWOBASE_D7_MAIL_USER, CVWOBASE_D7_MAIL_USER_DEFAULT)
	);
	$form['mail'][CVWOBASE_D7_MAIL_PASS] = array(
		'#type' => 'password',
		'#title' => t('SMTP Password for sending email'),
		'#default_value' => variable_get(CVWOBASE_D7_MAIL_PASS, CVWOBASE_D7_MAIL_PASS_DEFAULT)
	);
	$form['mail'][CVWOBASE_D7_MAIL_PORT] = array(
		'#type' => 'textfield',
		'#title' => t('SMTP Port for sending email'),
		'#description' => t('The port to use for your server. For more details, please contact your SMTP server administrator for help.'),
		'#default_value' => variable_get(CVWOBASE_D7_MAIL_PORT, CVWOBASE_D7_MAIL_PORT_DEFAULT)
	);
	return system_settings_form($form);
}

/**
 * Form validation handler for cvwobase_d7_admin_form().
 *
 * @see cvwobase_d7_admin_form()
 */
function cvwobase_d7_admin_form_validate($form, &$form_state) {
	$values = &$form_state['values'];
	
	$path = $values[CVWOBASE_D7_PHP_BINARY];
	if (empty($path))
		form_set_value($form['mail'][CVWOBASE_D7_PHP_BINARY], FALSE, $form_state);
	elseif (!is_readable($path) || !is_file($path))
		form_set_error(CVWOBASE_D7_PHP_BINARY, t('%path does not exist, is not a file or is inaccessible.', array('%path' => $path)));
	
	if (!is_numeric($values[CVWOBASE_D7_MAIL_PORT])) {
    form_set_error(CVWOBASE_D7_MAIL_PORT, t('You must enter a number for the SMTP Port.'));
  }
}