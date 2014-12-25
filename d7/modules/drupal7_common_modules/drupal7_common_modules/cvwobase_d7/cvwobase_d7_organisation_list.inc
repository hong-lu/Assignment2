<?php
/*
 * The MIT License
 *
 * Copyright 2007-2013 Computing for Volunteer Welfare Organizations (CVWO),
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

namespace CVWO\Base {

	use \CVWO\Base\Database as db;

	function organisation_list($form, &$form_state) {
		$search = new \CVWOBaseSearch(__NAMESPACE__ . '\organisation_list', $form);

		$input = array(
			'#type'			 => 'fieldset',
			'#title'		 => t('Basic Search'),
			'#collapsible'	 => true,
			'search_field'	 => array(
				'#type'		 => 'textfield',
				'#title'	 => t('Search organisation name')
			)
		);

		$header = array(
			array(
				'data' => t('Name'),
				'field' => 'name',
			),
			array(
				'data' => t('Type'),
				'field' => 'type',
			),
			array(
				'data' => t('Industry'),
				'field' => 'industry',
			)
		);

		$query = db\select(ORGANISATION_TABLE, 'o_t')
			->fields('o_t', array('organisation_id', 'name', 'type', 'industry'))
			->condition('o_t.is_deleted', 0);
		$search->defineBasicSearch($input, $query, $header, 'name LIKE :str OR name');

		//Define the advanced search. First start by showing the CodeM fields.
		$search->registerCodemFilter(array(
			ORGANISATION_TABLE => array(
				array('type',				 MODULE_NAME, ORGANISATION_TYPE_QUESTION),
				array('industry',			 MODULE_NAME, ORGANISATION_INDUSTRY_QUESTION)
			)
		));
		
		$search->setAdvancedSearchQuery($query);
		$search->defineAdvancedSearch($header, array(), 'organisation_id');
		
		return $search;
	}
	
	function organisation_list_submit($form, &$form_state) {
		$form_state['rebuild'] = true;
	}
}
